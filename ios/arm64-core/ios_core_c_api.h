#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Stable C-compatible state layout for Swift or other native front ends.
// GPR order: RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI, R8..R15.
typedef struct KytyIOSCoreCState {
    uint64_t gpr[16];
    uint64_t rip;
    uint64_t rflags;
    uint8_t* memory;
    size_t memory_size;
    uint64_t memory_base;
} KytyIOSCoreCState;

typedef enum KytyIOSCoreCResult {
    KYTY_IOS_CORE_EXECUTED = 0,
    KYTY_IOS_CORE_HALTED = 1,
    KYTY_IOS_CORE_INVALID_ARGUMENT = 2,
    KYTY_IOS_CORE_DECODE_ERROR = 3,
    KYTY_IOS_CORE_UNSUPPORTED_INSTRUCTION = 4,
    KYTY_IOS_CORE_MEMORY_FAULT = 5,
    KYTY_IOS_CORE_STEP_LIMIT = 6
} KytyIOSCoreCResult;

uint32_t kyty_ios_core_c_api_version(void);

// Executes a tiny, deterministic interpreter diagnostic. Returns 1 on pass, 0 on failure.
int kyty_ios_core_self_test(uint64_t* out_rax, size_t* out_steps);

uint32_t kyty_ios_core_c_api_version(void);

uint32_t kyty_ios_core_step(KytyIOSCoreCState* state,
                            const uint8_t* code,
                            size_t code_size,
                            uint8_t* instruction_length,
                            char* mnemonic,
                            size_t mnemonic_capacity);

uint32_t kyty_ios_core_run(KytyIOSCoreCState* state,
                           const uint8_t* code,
                           size_t code_size,
                           uint64_t code_base,
                           size_t max_steps,
                           size_t* steps_executed);

#ifdef __cplusplus
}
#endif
