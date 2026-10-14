# iOS ARM64 dependency probe

This probe builds the exact pinned Zydis decoder revision used by the desktop project as a native iOS ARM64 static dependency. Zydis decodes x86-64 instruction bytes; building it on ARM64 does not itself execute or translate those instructions.

The project currently does **not** build the full KytyPS5 core for iOS. The guest instruction emulator contains x86 host intrinsics and `cpuid.h`, while `src/loader/jit.h` emits x86-64 machine-code trampolines. Those pieces need a separate architecture-aware interpreter/dispatcher or a supported ARM64 translation backend before a full port is possible.

The CI job configures this dependency with the iPhoneOS SDK and compiles it for ARM64. Success proves only that this dependency can be built for iOS; it is not evidence of a working PS5 emulator.
