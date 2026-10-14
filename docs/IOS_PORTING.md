# KytyPS5 iOS port: feasibility and roadmap

## Current state

The upstream KytyPS5 project documents Windows x64 and Linux x64 as primary targets, with experimental macOS support. The macOS build is x86-64 and uses Rosetta 2. It is not an iOS build.

This branch starts a separate native iOS application shell. The shell is a milestone for app/toolchain packaging only; it does not contain a working PS5 emulator core and cannot run PS5 games.

## Why a direct build is not enough

- CPU architecture: iPhones use ARM64, while the existing desktop build expects x86-64 guest-code translation paths and desktop host assumptions. The actual guest CPU implementation must be inspected and adapted, not just recompiled.
- Graphics: the renderer targets Vulkan 1.3. iOS uses Metal. MoltenVK can provide Vulkan-on-Metal in some environments, but required extensions and features must be verified on iPhone hardware.
- Platform APIs and dependencies: SDL, Qt, filesystem access, threads, audio, input, dynamic loading, and other dependencies need iOS-compatible configurations or replacements.
- Execution and signing: iOS restricts executable memory/JIT behavior and app entitlements. Any JIT-dependent path needs a supported, tested approach.
- Memory and performance: PS5 games are designed for a much larger system and GPU. Even a successful app build may not run games at useful speed or at all.

## Milestones

1. Build and package the native SwiftUI shell on a macOS CI runner.
2. Create a minimal iOS ARM64 C/C++ toolchain probe and identify compile-time platform blockers.
3. Separate platform-independent emulator code from desktop-specific code.
4. Inventory x86-64 translation/JIT, Vulkan extensions, and OS API dependencies.
5. Port one isolated core component and add a small automated test.
6. Evaluate the graphics path against the iPhone's actual Metal capabilities.
7. Integrate more of the core only after earlier probes pass.
8. Produce a signed IPA only after a real iOS app target and signing route are available.
9. Test startup and legal user-supplied game data; report exact compatibility results.

## Build the prototype shell

On a macOS runner with Xcode, install XcodeGen, generate the project from ios/project.yml, then build the KytyPS5-iOS scheme with the iPhoneOS SDK and CODE_SIGNING_ALLOWED=NO.

The current workflow uploads the unsigned .app bundle as a CI artifact for inspection. An unsigned app bundle is not an installable IPA. Signing and installation are a later milestone.

## Project rules

- Do not include copyrighted PS5 games, keys, or console firmware.
- Do not describe a successful shell build as successful PS5 emulation.
- Keep changes focused and verify them on CI before claiming they work.
- Preserve upstream license notices and comply with GPL-2.0-only when redistributing modified binaries or source.
