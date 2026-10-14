#include "ios_core_c_api.h"

#include "x64_interpreter.h"

#include <algorithm>
#include <cstring>

namespace {

KytyIOSCore::X64State ToCppState(const KytyIOSCoreCState& input) {
    KytyIOSCore::X64State out{};
    out.rax = input.gpr[0];
    out.rcx = input.gpr[1];
    out.rdx = input.gpr[2];
    out.rbx = input.gpr[3];
    out.rsp = input.gpr[4];
    out.rbp = input.gpr[5];
    out.rsi = input.gpr[6];
    out.rdi = input.gpr[7];
    out.r8 = input.gpr[8];
    out.r9 = input.gpr[9];
    out.r10 = input.gpr[10];
    out.r11 = input.gpr[11];
    out.r12 = input.gpr[12];
    out.r13 = input.gpr[13];
    out.r14 = input.gpr[14];
    out.r15 = input.gpr[15];
    out.rip = input.rip;
    out.rflags = input.rflags;
    out.memory = input.memory;
    out.memory_size = input.memory_size;
    out.memory_base = input.memory_base;
    return out;
}

void FromCppState(const KytyIOSCore::X64State& input, KytyIOSCoreCState& out) {
    out.gpr[0] = input.rax;
    out.gpr[1] = input.rcx;
    out.gpr[2] = input.rdx;
    out.gpr[3] = input.rbx;
    out.gpr[4] = input.rsp;
    out.gpr[5] = input.rbp;
    out.gpr[6] = input.rsi;
    out.gpr[7] = input.rdi;
    out.gpr[8] = input.r8;
    out.gpr[9] = input.r9;
    out.gpr[10] = input.r10;
    out.gpr[11] = input.r11;
    out.gpr[12] = input.r12;
    out.gpr[13] = input.r13;
    out.gpr[14] = input.r14;
    out.gpr[15] = input.r15;
    out.rip = input.rip;
    out.rflags = input.rflags;
}

void CopyMnemonic(const char* source, char* destination, size_t capacity) {
    if (destination == nullptr || capacity == 0) {
        return;
    }
    const size_t count = std::min(std::strlen(source), capacity - 1);
    std::memcpy(destination, source, count);
    destination[count] = '\0';
}

} // namespace

extern "C" uint32_t kyty_ios_core_c_api_version(void) {
    return 1;
}

extern "C" uint32_t kyty_ios_core_step(KytyIOSCoreCState* state,
                                        const uint8_t* code,
                                        size_t code_size,
                                        uint8_t* instruction_length,
                                        char* mnemonic,
                                        size_t mnemonic_capacity) {
    if (state == nullptr) {
        CopyMnemonic("", mnemonic, mnemonic_capacity);
        if (instruction_length != nullptr) *instruction_length = 0;
        return KYTY_IOS_CORE_INVALID_ARGUMENT;
    }

    auto cppState = ToCppState(*state);
    const auto report = KytyIOSCore::Step(&cppState, code, code_size);
    FromCppState(cppState, *state);
    if (instruction_length != nullptr) *instruction_length = report.instruction_length;
    CopyMnemonic(report.mnemonic, mnemonic, mnemonic_capacity);
    return static_cast<uint32_t>(report.result);
}

extern "C" uint32_t kyty_ios_core_run(KytyIOSCoreCState* state,
                                       const uint8_t* code,
                                       size_t code_size,
                                       uint64_t code_base,
                                       size_t max_steps,
                                       size_t* steps_executed) {
    if (steps_executed != nullptr) *steps_executed = 0;
    if (state == nullptr) {
        return KYTY_IOS_CORE_INVALID_ARGUMENT;
    }

    auto cppState = ToCppState(*state);
    const auto report = KytyIOSCore::Run(&cppState, code, code_size, code_base, max_steps);
    FromCppState(cppState, *state);
    if (steps_executed != nullptr) *steps_executed = report.steps_executed;
    return static_cast<uint32_t>(report.result);
}
