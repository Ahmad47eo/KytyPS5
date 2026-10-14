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

void TestRegisterMove() {
    KytyIOSCore::X64State state{};
    state.rbx = 0x123456789ABCDEF0ULL;
    constexpr std::array<uint8_t, 3> code{0x48, 0x89, 0xD8}; // mov rax, rbx
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Executed &&
          state.rax == 0x123456789ABCDEF0ULL && state.rip == 3,
          "MOV register, register copies 64-bit value");
}

void TestXorRegisterRegister() {
    KytyIOSCore::X64State state{};
    state.rax = 0xFF00;
    state.rbx = 0x0F0F;
    constexpr std::array<uint8_t, 3> code{0x48, 0x31, 0xD8}; // xor rax, rbx
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Executed &&
          state.rax == 0xF00F && (state.rflags & (1ULL << 6)) == 0 &&
          (state.rflags & (1ULL << 0)) == 0 &&
          (state.rflags & (1ULL << 11)) == 0,
          "XOR computes result and clears CF/OF");
}

void TestRegisterRegisterAdd() {
    KytyIOSCore::X64State state{};
    state.rax = 12;
    state.rbx = 30;
    constexpr std::array<uint8_t, 3> code{0x48, 0x01, 0xD8}; // add rax, rbx
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Executed &&
          state.rax == 42 && state.rip == 3,
          "ADD register, register executes");
}

void TestCmpSetsZeroFlagWithoutChangingDestination() {
    KytyIOSCore::X64State state{};
    state.rax = 42;
    constexpr std::array<uint8_t, 4> code{0x48, 0x83, 0xF8, 0x2A}; // cmp rax, 42
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Executed &&
          state.rax == 42 && (state.rflags & (1ULL << 6)) != 0 &&
          state.rip == 4,
          "CMP sets ZF and leaves destination unchanged");
}

void TestCmpSetsCarryAndSignFlags() {
    KytyIOSCore::X64State state{};
    state.rax = 1;
    constexpr std::array<uint8_t, 4> code{0x48, 0x83, 0xF8, 0x02}; // cmp rax, 2
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Executed &&
          (state.rflags & (1ULL << 0)) != 0 &&
          (state.rflags & (1ULL << 7)) != 0 &&
          (state.rflags & (1ULL << 6)) == 0,
          "CMP sets carry/sign and clears zero when lhs is smaller");
}

void TestJzTaken() {
    KytyIOSCore::X64State state{};
    state.rip = 0x1000;
    state.rflags |= 1ULL << 6;
    constexpr std::array<uint8_t, 2> code{0x74, 0x05}; // jz +5
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Executed &&
          state.rip == 0x1007,
          "JZ takes branch when ZF is set");
}

void TestJzNotTaken() {
    KytyIOSCore::X64State state{};
    state.rip = 0x1000;
    constexpr std::array<uint8_t, 2> code{0x74, 0x05}; // jz +5
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Executed &&
          state.rip == 0x1002,
          "JZ falls through when ZF is clear");
}

void TestJnzTaken() {
    KytyIOSCore::X64State state{};
    state.rip = 0x1000;
    constexpr std::array<uint8_t, 2> code{0x75, 0xFC}; // jnz -4
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Executed &&
          state.rip == 0x0FFE,
          "JNZ applies signed backward displacement");
}

void TestAddOverflowFlag() {
    KytyIOSCore::X64State state{};
    state.rax = 0x7FFFFFFFFFFFFFFFULL;
    constexpr std::array<uint8_t, 4> code{0x48, 0x83, 0xC0, 0x01};
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::Executed &&
          state.rax == 0x8000000000000000ULL &&
          (state.rflags & (1ULL << 11)) != 0 &&
          (state.rflags & (1ULL << 7)) != 0,
          "ADD models signed overflow and sign flags");
}

void TestUnsupportedInstructionDoesNotMutateState() {
    KytyIOSCore::X64State state{};
    state.rax = 7;
    state.rbx = 9;
    state.rip = 0x3000;
    constexpr std::array<uint8_t, 3> code{0x48, 0x8D, 0x03}; // lea rax, [rbx]
    const auto report = KytyIOSCore::Step(&state, code.data(), code.size());
    Check(report.result == KytyIOSCore::StepResult::UnsupportedInstruction &&
          state.rax == 7 && state.rip == 0x3000,
          "Unsupported LEA leaves state unchanged");
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
    TestRegisterMove();
    TestXorRegisterRegister();
    TestRegisterRegisterAdd();
    TestCmpSetsZeroFlagWithoutChangingDestination();
    TestCmpSetsCarryAndSignFlags();
    TestJzTaken();
    TestJzNotTaken();
    TestJnzTaken();
    TestAddOverflowFlag();
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
