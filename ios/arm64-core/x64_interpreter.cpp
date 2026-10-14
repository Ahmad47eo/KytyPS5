#include "x64_interpreter.h"

#include <Zydis/Zydis.h>
#include <algorithm>
#include <cstring>

namespace KytyIOSCore {
namespace {

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
        case ZYDIS_MNEMONIC_RETN:
            return Report(StepResult::Halted, ins.length, "ret");

        case ZYDIS_MNEMONIC_MOV: {
            if (ins.operand_count_visible < 2 ||
                operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
                operands[1].type != ZYDIS_OPERAND_TYPE_IMMEDIATE) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "mov");
            }
            auto* dst = RegisterPointer(*state, operands[0].reg.value);
            if (dst == nullptr || operands[0].size != 64) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "mov");
            }
            *dst = static_cast<uint64_t>(operands[1].imm.value.u);
            state->rip += ins.length;
            return Report(StepResult::Executed, ins.length, "mov");
        }

        case ZYDIS_MNEMONIC_ADD:
        case ZYDIS_MNEMONIC_SUB: {
            if (ins.operand_count_visible < 2 ||
                operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
                operands[1].type != ZYDIS_OPERAND_TYPE_IMMEDIATE ||
                operands[0].size != 64) {
                return Report(StepResult::UnsupportedInstruction, ins.length,
                              ins.mnemonic == ZYDIS_MNEMONIC_ADD ? "add" : "sub");
            }
            auto* dst = RegisterPointer(*state, operands[0].reg.value);
            if (dst == nullptr) {
                return Report(StepResult::UnsupportedInstruction, ins.length, "arith");
            }
            const uint64_t old = *dst;
            const uint64_t imm = static_cast<uint64_t>(operands[1].imm.value.u);
            *dst = ins.mnemonic == ZYDIS_MNEMONIC_ADD ? old + imm : old - imm;
            // Arithmetic flags are intentionally not modelled in this first milestone.
            state->rip += ins.length;
            return Report(StepResult::Executed, ins.length,
                          ins.mnemonic == ZYDIS_MNEMONIC_ADD ? "add" : "sub");
        }

        default:
            return Report(StepResult::UnsupportedInstruction, ins.length, "unsupported");
    }
}

extern "C" uint32_t kyty_ios_core_abi_version() {
    return 1;
}

} // namespace KytyIOSCore
