# KytyPS5 iOS port: feasibility and roadmap

## Current state

The upstream KytyPS5 project documents Windows x64 and Linux x64 as primary targets, with experimental macOS support. The macOS build is x86-64 and uses Rosetta 2. It is not an iOS build.

This branch contains a native SwiftUI app shell and a separate C++20 iOS ARM64 toolchain probe. The shell is only a packaging milestone; it does not contain a working PS5 emulator core and cannot run PS5 games. The C++ probe checks that the Apple toolchain can compile a small native ARM64 static library for iPhoneOS.

## Why a direct build is not enough

- CPU architecture: iPhones use ARM64, while the existing desktop build expects x86-64 guest-code translation paths and desktop host assumptions. The actual guest CPU implementation must be inspected and adapted, not just recompiled.
- Graphics: the renderer targets Vulkan 1.3. iOS uses Metal. MoltenVK can provide Vulkan-on-Metal in some environments, but required extensions and features must be verified on iPhone hardware.
- Platform APIs and dependencies: SDL, Qt, filesystem access, threads, audio, input, dynamic loading, and other dependencies need iOS-compatible configurations or replacements.
- Execution and signing: iOS restricts executable memory/JIT behavior and app entitlements. Any JIT-dependent path needs a supported, tested approach.
- Memory and performance: PS5 games are designed for a much larger system and GPU. Even a successful app build may not run games at useful speed or at all.

## Milestones

1. Build and package the native SwiftUI shell on a macOS CI runner.
2. Compile a minimal native C++20 static library for iOS ARM64.
3. Separate platform-independent emulator code from desktop-specific code.
4. Inventory x86-64 translation/JIT, Vulkan extensions, and OS API dependencies.
5. Port one isolated core component and add a small automated test.
6. Evaluate the graphics path against the iPhone's actual Metal capabilities.
7. Integrate more of the core only after earlier probes pass.
8. Produce a signed IPA only after a real iOS app target and signing route are available.
9. Test startup and legal user-supplied game data; report exact compatibility results.

## CI build

The iOS Port Prototype workflow generates the Xcode project, compiles the unsigned SwiftUI shell, then configures and compiles the ARM64 C++ probe with the iPhoneOS SDK. It uploads both build artifacts for inspection.

The unsigned .app bundle is not an installable IPA. Signing and installation are a later milestone.

## Project rules

- Do not include copyrighted PS5 games, keys, or console firmware.
- Do not describe a successful shell build or toolchain probe as successful PS5 emulation.
- Keep changes focused and verify them on CI before claiming they work.
- Preserve upstream license notices and comply with GPL-2.0-only when redistributing modified binaries or source.


## Interpreter milestone: bounded guest memory

The prototype interpreter now supports a deliberately small flat guest-memory window attached to `X64State`. The window maps guest virtual addresses beginning at `memory_base` onto a caller-owned byte buffer of `memory_size` bytes. The current implementation uses checked bounds and `memcpy` for 64-bit MOV loads/stores, returning `MemoryFault` for missing or out-of-range memory instead of dereferencing an invalid host pointer.

The supported instruction subset now includes NOP, 64-bit MOV immediate/register/memory forms, register/immediate ADD and SUB, CMP, XOR, and JMP/JZ/JNZ. Arithmetic flags are still a partial implementation, and many architectural details are not implemented. RET still halts the prototype rather than popping a guest return address. This remains a test harness, not a connected PS5 runtime.

Next engineering steps:
1. Validate all new memory tests on the macOS Actions runner and fix any portability or decoder mismatches.
2. Add width-correct 8/16/32-bit register semantics and memory forms with precise x86 zero-extension/sign-extension rules.
3. Add stack operations and a real guest return path, then build a small execution loop with instruction and fault limits.
4. Design a proper guest address-space abstraction before connecting any upstream loader code.
