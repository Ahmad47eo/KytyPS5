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

bool EffectiveAddress(const X64State& state, const ZydisDecodedInstruction& ins,
                      const ZydisDecodedOperand& operand, uint64_t& address) {
    if (operand.type != ZYDIS_OPERAND_TYPE_MEMORY || operand.size != 64) {
        return false;
    }

    uint64_t base = 0;
    if (operand.mem.base == ZYDIS_REGISTER_RIP ||
        operand.mem.base == ZYDIS_REGISTER_EIP) {
        base = state.rip + ins.length;
    } else if (operand.mem.base != ZYDIS_REGISTER_NONE) {
        const auto* reg = RegisterPointer(const_cast<X64State&>(state), operand.mem.base);
        if (reg == nullptr) return false;
        base = *reg;
    }

    uint64_t index = 0;
    if (operand.mem.index != ZYDIS_REGISTER_NONE) {
        const auto* reg = RegisterPointer(const_cast<X64State&>(state), operand.mem.index);
        if (reg == nullptr) return false;
        index = *reg * operand.mem.scale;
    }

    address = base + index + static_cast<uint64_t>(operand.mem.disp.value);
    return true;
}

bool MemoryOffset(const X64State& state, uint64_t address, size_t& offset) {
    if (state.memory == nullptr || address < state.memory_base) return false;
    const uint64_t delta = address - state.memory_base;
    if (delta > state.memory_size || state.memory_size - static_cast<size_t>(delta) < sizeof(uint64_t)) {
        return false;
    }
    offset = static_cast<size_t>(delta);
    return true;
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
        case ZYDIS_MNEMONIC_PUSH: {
            if (ins.operand_count_visible < 1 || operands[0].size != 64) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "push");
            }
            uint64_t value = 0;
            if (!Read64BitSource(*state, operands[0], value)) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "push");
            }
            if (state->rsp < sizeof(value)) {
                return Report(StepResult::MemoryFault, ins.length, "push");
            }
            const uint64_t new_rsp = state->rsp - sizeof(value);
            size_t offset = 0;
            if (!MemoryOffset(*state, new_rsp, offset)) {
                return Report(StepResult::MemoryFault, ins.length, "push");
            }
            std::memcpy(state->memory + offset, &value, sizeof(value));
            state->rsp = new_rsp;
            state->rip += ins.length;
            return Report(StepResult::Executed, ins.length, "push");
        }

        case ZYDIS_MNEMONIC_POP: {
            if (ins.operand_count_visible < 1 ||
                operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
                operands[0].size != 64) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "pop");
            }
            auto* dst = RegisterPointer(*state, operands[0].reg.value);
            if (dst == nullptr) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "pop");
            }
            size_t offset = 0;
            if (!MemoryOffset(*state, state->rsp, offset)) {
                return Report(StepResult::MemoryFault, ins.length, "pop");
            }
            uint64_t value = 0;
            std::memcpy(&value, state->memory + offset, sizeof(value));
            const uint64_t next_rsp = state->rsp + sizeof(value);
            if (dst == &state->rsp) {
                // POP RSP loads the popped value into RSP; the implicit stack
                // increment does not overwrite that destination value.
                state->rsp = value;
            } else {
                *dst = value;
                state->rsp = next_rsp;
            }
            state->rip += ins.length;
            return Report(StepResult::Executed, ins.length, "pop");
        }

        case ZYDIS_MNEMONIC_NOP:
            state->rip += ins.length;
            return Report(StepResult::Executed, ins.length, "nop");

        case ZYDIS_MNEMONIC_RET: {
            size_t offset = 0;
            if (!MemoryOffset(*state, state->rsp, offset)) {
                return Report(StepResult::MemoryFault, ins.length, "ret");
            }
            uint64_t return_address = 0;
            std::memcpy(&return_address, state->memory + offset, sizeof(return_address));
            state->rsp += sizeof(return_address);
            state->rip = return_address;
            return Report(StepResult::Executed, ins.length, "ret");
        }

        case ZYDIS_MNEMONIC_CALL: {
            if (ins.operand_count_visible < 1 ||
                operands[0].type != ZYDIS_OPERAND_TYPE_IMMEDIATE) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "call");
            }
            if (state->rsp < sizeof(uint64_t)) {
                return Report(StepResult::MemoryFault, ins.length, "call");
            }
            const uint64_t new_rsp = state->rsp - sizeof(uint64_t);
            size_t offset = 0;
            if (!MemoryOffset(*state, new_rsp, offset)) {
                return Report(StepResult::MemoryFault, ins.length, "call");
            }
            const uint64_t return_address = state->rip + ins.length;
            std::memcpy(state->memory + offset, &return_address, sizeof(return_address));
            state->rsp = new_rsp;
            state->rip = static_cast<uint64_t>(
                static_cast<int64_t>(return_address) + operands[0].imm.value.s);
            return Report(StepResult::Executed, ins.length, "call");
        }

        case ZYDIS_MNEMONIC_LEA: {
            if (ins.operand_count_visible < 2 ||
                operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
                operands[0].size != 64 ||
                operands[1].type != ZYDIS_OPERAND_TYPE_MEMORY ||
                operands[1].size != 64) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "lea");
            }
            auto* dst = RegisterPointer(*state, operands[0].reg.value);
            uint64_t address = 0;
            if (dst == nullptr || !EffectiveAddress(*state, ins, operands[1], address)) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "lea");
            }
            *dst = address;
            state->rip += ins.length;
            return Report(StepResult::Executed, ins.length, "lea");
        }

        case ZYDIS_MNEMONIC_MOV: {
            if (ins.operand_count_visible < 2) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "mov");
            }

            // Register destination: immediate/register source or bounded guest-memory load.
            if (operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                operands[0].size == 64) {
                auto* dst = RegisterPointer(*state, operands[0].reg.value);
                if (dst == nullptr) {
                    return Report(StepResult::UnsupportedInstruction, ins.length, "mov");
                }
                uint64_t value = 0;
                if (operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY) {
                    uint64_t address = 0;
                    size_t offset = 0;
                    if (!EffectiveAddress(*state, ins, operands[1], address)) {
                        return Report(StepResult::UnsupportedInstruction, ins.length, "mov");
                    }
                    if (!MemoryOffset(*state, address, offset)) {
                        return Report(StepResult::MemoryFault, ins.length, "mov");
                    }
                    std::memcpy(&value, state->memory + offset, sizeof(value));
                } else if (!Read64BitSource(*state, operands[1], value)) {
                    return Report(StepResult::UnsupportedInstruction, ins.length, "mov");
                }
                *dst = value;
                state->rip += ins.length;
                return Report(StepResult::Executed, ins.length, "mov");
            }

            // Bounded guest-memory store from a 64-bit register.
            if (operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                operands[0].size == 64 &&
                operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                operands[1].size == 64) {
                const auto* src = RegisterPointer(*state, operands[1].reg.value);
                uint64_t address = 0;
                size_t offset = 0;
                if (src == nullptr ||
                    !EffectiveAddress(*state, ins, operands[0], address)) {
                    return Report(StepResult::UnsupportedInstruction, ins.length, "mov");
                }
                if (!MemoryOffset(*state, address, offset)) {
                    return Report(StepResult::MemoryFault, ins.length, "mov");
                }
                std::memcpy(state->memory + offset, src, sizeof(*src));
                state->rip += ins.length;
                return Report(StepResult::Executed, ins.length, "mov");
            }

            return Report(StepResult::UnsupportedInstruction, ins.length, "mov");
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

RunReport Run(X64State* state, const uint8_t* code, size_t code_size,
              uint64_t code_base, size_t max_steps) {
    RunReport out{};
    out.result = StepResult::InvalidArgument;
    out.last_step = Report(StepResult::InvalidArgument, 0, "");

    if (state == nullptr || code == nullptr || code_size == 0) {
        return out;
    }

    for (size_t count = 0; count < max_steps; ++count) {
        if (state->rip < code_base) {
            out.result = StepResult::MemoryFault;
            out.steps_executed = count;
            return out;
        }
        const uint64_t delta = state->rip - code_base;
        if (delta >= code_size) {
            out.result = StepResult::MemoryFault;
            out.steps_executed = count;
            return out;
        }

        const size_t offset = static_cast<size_t>(delta);
        out.last_step = Step(state, code + offset, code_size - offset);
        if (out.last_step.result != StepResult::Executed) {
            out.result = out.last_step.result;
            out.steps_executed = count + 1;
            return out;
        }
        out.steps_executed = count + 1;
    }

    out.result = StepResult::StepLimit;
    return out;
}

extern "C" uint32_t kyty_ios_core_abi_version() {
    return 3;
}

} // namespace KytyIOSCore
