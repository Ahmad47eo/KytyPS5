#pragma once

#include <cstddef>
#include <cstdint>

namespace KytyIOSCore {

// Minimal bring-up interpreter state. This is a diagnostic foundation, not a PS5 CPU.
struct X64State {
    uint64_t rax = 0;
    uint64_t rcx = 0;
    uint64_t rdx = 0;
    uint64_t rbx = 0;
    uint64_t rsp = 0;
    uint64_t rbp = 0;
    uint64_t rsi = 0;
    uint64_t rdi = 0;
    uint64_t r8 = 0;
    uint64_t r9 = 0;
    uint64_t r10 = 0;
    uint64_t r11 = 0;
    uint64_t r12 = 0;
    uint64_t r13 = 0;
    uint64_t r14 = 0;
    uint64_t r15 = 0;
    uint64_t rip = 0;
    uint64_t rflags = 0x2;

    // Optional flat guest-memory window. Guest addresses in [memory_base,
    // memory_base + memory_size) map to this host buffer; no host pointer is
    // exposed to guest code.
    uint8_t* memory = nullptr;
    size_t memory_size = 0;
    uint64_t memory_base = 0;
};

enum class StepResult : uint32_t {
    Executed = 0,
    Halted = 1,
    InvalidArgument = 2,
    DecodeError = 3,
    UnsupportedInstruction = 4,
    MemoryFault = 5,
    StepLimit = 6
};

struct StepReport {
    StepResult result;
    uint8_t instruction_length;
    char mnemonic[32];
};

struct RunReport {
    StepResult result;
    size_t steps_executed;
    StepReport last_step;
};

// Executes one explicitly supported x86-64 instruction using an instruction decoder.
// code points to readable guest instruction bytes; this bring-up core does not JIT.
StepReport Step(X64State* state, const uint8_t* code, size_t code_size);

// Executes instructions from a contiguous guest-code window until a fault,
// halt, leaving the window, or max_steps is reached.
RunReport Run(X64State* state, const uint8_t* code, size_t code_size,
              uint64_t code_base, size_t max_steps);

// Stable C ABI so a future Swift shell can query the core build version.
extern "C" uint32_t kyty_ios_core_abi_version();

} // namespace KytyIOSCore
