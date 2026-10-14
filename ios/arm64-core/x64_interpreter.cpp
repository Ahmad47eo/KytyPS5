#include "x64_interpreter.h"

#include <Zydis/Zydis.h>
#include <cstring>

namespace KytyIOSCore {
namespace {

constexpr uint64_t FLAG_CF = 1ULL << 0;
constexpr uint64_t FLAG_PF = 1ULL << 2;
constexpr uint64_t FLAG_ZF = 1ULL << 6;
constexpr uint64_t FLAG_SF = 1ULL << 7;
constexpr uint64_t FLAG_OF = 1ULL << 11;
constexpr uint64_t ARITHMETIC_FLAGS = FLAG_CF | FLAG_PF | FLAG_ZF | FLAG_SF | FLAG_OF;

uint64_t* RegisterPointer(X64State& s, ZydisRegister r) {
    switch (r) {
        case ZYDIS_REGISTER_RAX: return &s.rax;
        case ZYDIS_REGISTER_RCX: return &s.rcx;
        case ZYDIS_REGISTER_RDX: return &s.rdx;
        case ZYDIS_REGISTER_RBX: return &s.rbx;
        case ZYDIS_REGISTER_RSP: return &s.rsp;
        case ZYDIS_REGISTER_RBP: return &s.rbp;
        case ZYDIS_REGISTER_RSI: return &s.rsi;
        case ZYDIS_REGISTER_RDI: return &s.rdi;
        case ZYDIS_REGISTER_R8: return &s.r8;
        case ZYDIS_REGISTER_R9: return &s.r9;
        case ZYDIS_REGISTER_R10: return &s.r10;
        case ZYDIS_REGISTER_R11: return &s.r11;
        case ZYDIS_REGISTER_R12: return &s.r12;
        case ZYDIS_REGISTER_R13: return &s.r13;
        case ZYDIS_REGISTER_R14: return &s.r14;
        case ZYDIS_REGISTER_R15: return &s.r15;
        default: return nullptr;
    }
}

StepReport Report(StepResult result, uint8_t length, const char* name) {
    StepReport out{};
    out.result = result;
    out.instruction_length = length;
    if (name != nullptr) {
        std::strncpy(out.mnemonic, name, sizeof(out.mnemonic) - 1);
    }
    return out;
}

bool Read64BitSource(X64State& state, const ZydisDecodedOperand& operand, uint64_t& value) {
    if (operand.type == ZYDIS_OPERAND_TYPE_REGISTER && operand.size == 64) {
        const auto* reg = RegisterPointer(state, operand.reg.value);
        if (reg == nullptr) {
            return false;
        }
        value = *reg;
        return true;
    }
    if (operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
        // Zydis marks sign-extended immediates as signed; preserving that matters
        // for encodings such as ADD RAX, -1.
        value = operand.imm.is_signed
                    ? static_cast<uint64_t>(operand.imm.value.s)
                    : operand.imm.value.u;
        return true;
    }
    return false;
}

void SetArithmeticFlags(X64State& state, uint64_t lhs, uint64_t rhs,
                        uint64_t result, bool subtract, bool carry) {
    uint64_t flags = state.rflags & ~ARITHMETIC_FLAGS;
    if (carry) {
        flags |= FLAG_CF;
    }
    if (result == 0) {
        flags |= FLAG_ZF;
    }
    if ((result >> 63) != 0) {
        flags |= FLAG_SF;
    }
    // x86 parity is even parity of the low result byte.
    uint8_t low = static_cast<uint8_t>(result);
    low ^= static_cast<uint8_t>(low >> 4);
    low &= 0x0F;
    if (((0x6996U >> low) & 1U) == 0) {
        flags |= FLAG_PF;
    }
    const bool overflow = subtract
        ? (((lhs ^ rhs) & (lhs ^ result) & (1ULL << 63)) != 0)
        : (((~(lhs ^ rhs)) & (lhs ^ result) & (1ULL << 63)) != 0);
    if (overflow) {
        flags |= FLAG_OF;
    }
    state.rflags = flags | 0x2;
}

bool IsZero(const X64State& state) {
    return (state.rflags & FLAG_ZF) != 0;
}

} // namespace

StepReport Step(X64State* state, const uint8_t* code, size_t code_size) {
    if (state == nullptr || code == nullptr || code_size == 0) {
        return Report(StepResult::InvalidArgument, 0, "");
    }

    ZydisDecoder decoder{};
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64,
                                       ZYDIS_STACK_WIDTH_64))) {
        return Report(StepResult::DecodeError, 0, "");
    }

    ZydisDecodedInstruction ins{};
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
    if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, code, code_size, &ins, operands))) {
        return Report(StepResult::DecodeError, 0, "");
    }

    switch (ins.mnemonic) {
        case ZYDIS_MNEMONIC_NOP:
            state->rip += ins.length;
            return Report(StepResult::Executed, ins.length, "nop");

        case ZYDIS_MNEMONIC_RET:
            return Report(StepResult::Halted, ins.length, "ret");

        case ZYDIS_MNEMONIC_MOV: {
            if (ins.operand_count_visible < 2 ||
                operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
                operands[0].size != 64) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "mov");
            }
            auto* dst = RegisterPointer(*state, operands[0].reg.value);
            uint64_t value = 0;
            if (dst == nullptr || !Read64BitSource(*state, operands[1], value)) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "mov");
            }
            *dst = value;
            state->rip += ins.length;
            return Report(StepResult::Executed, ins.length, "mov");
        }

        case ZYDIS_MNEMONIC_XOR: {
            if (ins.operand_count_visible < 2 ||
                operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
                operands[0].size != 64) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "xor");
            }
            auto* dst = RegisterPointer(*state, operands[0].reg.value);
            uint64_t rhs = 0;
            if (dst == nullptr || !Read64BitSource(*state, operands[1], rhs)) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "xor");
            }
            const uint64_t result = *dst ^ rhs;
            uint64_t flags = state->rflags & ~(ARITHMETIC_FLAGS);
            if (result == 0) flags |= FLAG_ZF;
            if ((result >> 63) != 0) flags |= FLAG_SF;
            uint8_t low = static_cast<uint8_t>(result);
            low ^= static_cast<uint8_t>(low >> 4);
            low &= 0x0F;
            if (((0x6996U >> low) & 1U) == 0) flags |= FLAG_PF;
            state->rflags = flags | 0x2;
            *dst = result;
            state->rip += ins.length;
            return Report(StepResult::Executed, ins.length, "xor");
        }

        case ZYDIS_MNEMONIC_ADD:
        case ZYDIS_MNEMONIC_SUB:
        case ZYDIS_MNEMONIC_CMP: {
            const char* name = ins.mnemonic == ZYDIS_MNEMONIC_ADD ? "add" :
                               ins.mnemonic == ZYDIS_MNEMONIC_SUB ? "sub" : "cmp";
            if (ins.operand_count_visible < 2 ||
                operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
                operands[0].size != 64) {
                return Report(StepResult::UnsupportedInstruction, ins.length, name);
            }
            auto* dst = RegisterPointer(*state, operands[0].reg.value);
            uint64_t rhs = 0;
            if (dst == nullptr || !Read64BitSource(*state, operands[1], rhs)) {
                return Report(StepResult::UnsupportedInstruction, ins.length, name);
            }
            const uint64_t lhs = *dst;
            const bool subtract = ins.mnemonic != ZYDIS_MNEMONIC_ADD;
            const uint64_t result = subtract ? lhs - rhs : lhs + rhs;
            const bool carry = subtract ? lhs < rhs : result < lhs;
            SetArithmeticFlags(*state, lhs, rhs, result, subtract, carry);
            if (ins.mnemonic != ZYDIS_MNEMONIC_CMP) {
                *dst = result;
            }
            state->rip += ins.length;
            return Report(StepResult::Executed, ins.length, name);
        }

        case ZYDIS_MNEMONIC_JMP:
        case ZYDIS_MNEMONIC_JZ:
        case ZYDIS_MNEMONIC_JNZ: {
            if (ins.operand_count_visible < 1 ||
                operands[0].type != ZYDIS_OPERAND_TYPE_IMMEDIATE) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "jump");
            }
            bool take = ins.mnemonic == ZYDIS_MNEMONIC_JMP;
            if (ins.mnemonic == ZYDIS_MNEMONIC_JZ) {
                take = IsZero(*state);
            } else if (ins.mnemonic == ZYDIS_MNEMONIC_JNZ) {
                take = !IsZero(*state);
            }
            const uint64_t next = state->rip + ins.length;
            state->rip = take
                ? static_cast<uint64_t>(static_cast<int64_t>(next) + operands[0].imm.value.s)
                : next;
            return Report(StepResult::Executed, ins.length,
                          ins.mnemonic == ZYDIS_MNEMONIC_JMP ? "jmp" :
                          ins.mnemonic == ZYDIS_MNEMONIC_JZ ? "jz" : "jnz");
        }

        default:
            return Report(StepResult::UnsupportedInstruction, ins.length, "unsupported");
    }
}

extern "C" uint32_t kyty_ios_core_abi_version() {
    return 2;
}

} // namespace KytyIOSCore
