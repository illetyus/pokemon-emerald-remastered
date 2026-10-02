# R0 Evidence Log

This document records observed R0 results. It is not the final architecture decision; the final decision belongs in an ADR after Android hardware testing.

## Common contract

All candidates use the synthetic R0 scenario defined in:

`shared/scenarios/r0_walk_event_encounter.json`

Expected final gameplay truth:

- tile: (3, 4)
- successful steps: 5
- interactions: 1
- event flags: 0x1
- encounter pending: true
- authoritative C-state hash: `0x642df4e66c5448a0`

## Candidate A — portable C core + SDL3

Implemented:

- SDL 3.4.16 pinned by archive SHA-256.
- Shared C gameplay core linked directly.
- Keyboard input.
- Gamepad D-pad/action input.
- Touch-event mapping.
- Interactive synthetic renderer.
- In-memory save/load.
- Windowless `--self-test`.
- Windows build CI.
- Canonical C-state hash verification.

Observed:

- Native SDL3 prototype compiles successfully in GitHub Actions on Windows.
- Deterministic executable self-test passes.
- Canonical C-state hash verification passes.
- Presentation event parity checks pass.

## Candidate B — portable C core + Godot presentation bridge

Implemented:

- Godot 4.7.2 stable.
- godot-cpp 10.0.0-stable pinned to commit `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`.
- GDExtension class `RemasterCoreBridge`.
- Narrow API: reset, step, snapshot, save, load, hash.
- Godot does not directly mutate authoritative gameplay state.
- Keyboard, gamepad and touch-event mapping.
- Headless equivalence test.
- Canonical C-state hash verification.
- Linux bridge CI.

Observed:

- CMake configuration with Godot 4.7 API targeting succeeds.
- Trimmed godot-cpp bindings and the GDExtension shared library build successfully.
- Godot 4.7.2 loads the generated extension in headless CI.
- Runtime equivalence test passes through the Godot-to-C boundary.
- Canonical C-state hash, save/load continuation and presentation event masks survive the bridge unchanged.

## Candidate C — pure Godot reimplementation

Implemented:

- Synthetic gameplay behavior reimplemented in GDScript.
- Collision, movement, event and encounter behavior.
- Snapshot/save/load comparison.
- Keyboard, gamepad and touch-event mapping.
- Headless test on Godot 4.7.2.
- Linux CI.

Observed:

- Godot 4.7.2 headless equivalence test passes in GitHub Actions.
- The implementation necessarily duplicates gameplay rules that already exist in the C/Vanilla+ lineage.
- Because it has its own gameplay implementation, field-by-field equivalence tests are required rather than inheriting the C-state hash implementation.

## Current engineering observation

Candidate C establishes that a pure Godot implementation is technically straightforward for the presentation/runtime layer, but it also demonstrates the core architectural cost: gameplay truth exists twice during migration.

Candidates A and B keep one authoritative gameplay core. Candidate B additionally exposes Godot's scene/UI/2D rendering workflow without transferring gameplay ownership to Godot.

Desktop R0 evidence now passes for all three candidates.

Candidate A and Candidate B advance to Android finalist testing because both preserve one authoritative gameplay core. Candidate C remains the control/comparison implementation: it proves Godot-only development is viable, but duplicates gameplay truth and would require reimplementing the existing Vanilla+ codebase.

No production architecture is selected until Android build/device evidence is complete.


## Android CI evidence

Both authoritative-core finalists now produce ARM64 Android artifacts.

### SDL3 + C core

- Android debug APK build passes.
- Package: `com.illetyus.emeraldremaster.r0sdl`
- APK size from the verified R0 artifact: 1,224,182 bytes.
- APK SHA-256: `0ad92c204086133e162cae42705f5f5aafe06927254e09a6916168b4cfb021e9`
- Lifecycle persistence uses `SDL_AddEventWatch()` for Android background events.
- Persistent state is written under `SDL_GetPrefPath()` with a magic header and state hash.
- Five-second frame-pacing telemetry is emitted as `R0 PERF renderer=SDL3 ...`.

### Godot + C core

- Android ARM64 GDExtension build passes.
- Android debug APK export passes.
- Package: `com.illetyus.emeraldremaster.r0godot`
- APK size from the verified R0 artifact: 37,876,706 bytes.
- APK SHA-256: `4a5482fbcf6d0226103cb6eb2f5d600f100c9a1b2229f468620147f14397ab59`
- APK inspection verifies the ARM64 GDExtension is packaged.
- Lifecycle persistence uses `NOTIFICATION_APPLICATION_PAUSED` and `user://`.
- Persistent save/load is hash-validated and covered by headless CI.
- Five-second frame-pacing telemetry is emitted as `R0 PERF renderer=Godot ...`.

The APK size comparison is not a production-size verdict: the Godot R0 package currently uses a debugging-oriented engine template while the SDL3 prototype is minimal synthetic code. Production size optimization is outside R0.

### Remaining evidence

The remaining R0 architecture gate is real-device testing with the same ADB protocol:

- install;
- canonical input sequence;
- pause/resume;
- force-stop/relaunch;
- persistent hash validation;
- touch/gamepad behavior;
- frame-pacing observations.

See `docs/R0_ANDROID_DEVICE_TEST.md`.
