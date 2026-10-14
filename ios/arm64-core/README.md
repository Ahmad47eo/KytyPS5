# Kyty iOS x86-64 interpreter bring-up

This is the first executable CPU-core experiment for the iOS port. It decodes x86-64 guest bytes with the pinned Zydis decoder and interprets a deliberately small initial subset: NOP, RET, 64-bit MOV register/immediate, and 64-bit ADD/SUB register/immediate.

## Current limits

- It is **not** the full KytyPS5 CPU engine and cannot run PS5 games.
- It does not yet implement flags, memory operands, branches, exceptions, syscalls, vector/SSE/AVX instructions, guest address-space management, or the PlayStation 5 runtime.
- RET reports a halt; it does not yet read a guest stack.
- This path interprets instructions instead of generating executable ARM64 code, avoiding a JIT requirement for this first test.
- The full upstream project is not yet wired into this target.

Next steps: add unit tests and flags, implement safe guest memory access, then cover control flow and the project's actual runtime requirements before attempting to link the emulator's real modules.
