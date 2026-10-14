#include "x64_interpreter.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void Check(bool condition, std::string_view name) {
    if (condition) {
        std::cout << "PASS: " << name << '\n';
    } else {
        std::cerr << "FAIL: " << name << '\n';
        ++failures;
    }
}

void TestNopAdvancesRip() {
    KytyIOSCore::X64State state{};
    state.rip = 0x1000;
    constexpr std::array<uint8_t, 1> code{0x90};
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Executed &&
          report.instruction_length == 1 && state.rip == 0x1001,
          "NOP executes and advances RIP");
}

void TestMovImmediate() {
    KytyIOSCore::X64State state{};
    constexpr std::array<uint8_t, 10> code{
        0x48, 0xB8, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
    };
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Executed &&
          state.rax == 0x1122334455667788ULL && state.rip == 10,
          "MOV RAX, imm64 writes register and advances RIP");
}

void TestAddImmediate() {
    KytyIOSCore::X64State state{};
    state.rax = 40;
    constexpr std::array<uint8_t, 4> code{0x48, 0x83, 0xC0, 0x02};
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Executed &&
          state.rax == 42 && state.rip == 4,
          "ADD RAX, imm8 updates register and RIP");
}

void TestSubImmediate() {
    KytyIOSCore::X64State state{};
    state.rax = 42;
    constexpr std::array<uint8_t, 4> code{0x48, 0x83, 0xE8, 0x02};
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Executed &&
          state.rax == 40 && state.rip == 4,
          "SUB RAX, imm8 updates register and RIP");
}

void TestRetHalts() {
    KytyIOSCore::X64State state{};
    state.rip = 0x2000;
    constexpr std::array<uint8_t, 1> code{0xC3};
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Halted &&
          report.instruction_length == 1 && state.rip == 0x2000,
          "RET reports halt without changing RIP");
}

void TestUnsupportedInstructionDoesNotMutateState() {
    KytyIOSCore::X64State state{};
    state.rax = 7;
    state.rbx = 9;
    state.rip = 0x3000;
    constexpr std::array<uint8_t, 3> code{0x48, 0x01, 0xD8}; // add rax, rbx
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::UnsupportedInstruction &&
          state.rax == 7 && state.rip == 0x3000,
          "Unsupported register-register ADD leaves state unchanged");
}

void TestInvalidArguments() {
    KytyIOSCore::X64State state{};
    constexpr std::array<uint8_t, 1> code{0x90};
    const auto nullState = KytyIOSCore::Step(nullptr, code.data(), code.size());
    const auto nullCode = KytyIOSCore::Step(&state, nullptr, code.size());
    const auto emptyCode = KytyIOSCore::Step(&state, code.data(), 0);
    Check(nullState.result == KytyIOSCore::StepResult::InvalidArgument &&
          nullCode.result == KytyIOSCore::StepResult::InvalidArgument &&
          emptyCode.result == KytyIOSCore::StepResult::InvalidArgument,
          "Null pointers and empty code are rejected");
}

void TestDecodeError() {
    KytyIOSCore::X64State state{};
    constexpr std::array<uint8_t, 1> truncatedMov{0x48};
    const auto report = KytyIOSCore::Step(&state, truncatedMov.data(), truncatedMov.size());
    Check(report.result == KytyIOSCore::StepResult::DecodeError,
          "Truncated instruction reports decode error");
}

} // namespace

int main() {
    TestNopAdvancesRip();
    TestMovImmediate();
    TestAddImmediate();
    TestSubImmediate();
    TestRetHalts();
    TestUnsupportedInstructionDoesNotMutateState();
    TestInvalidArguments();
    TestDecodeError();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "All interpreter tests passed\n";
    return 0;
}
