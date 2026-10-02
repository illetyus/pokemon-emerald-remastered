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

Observed so far:

- Native SDL3 prototype compiles successfully in GitHub Actions on Windows.
- Earlier deterministic self-test builds have completed successfully.
- Final post-hash CI remains part of the active R0 gate.

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

Observed so far:

- CMake configuration with Godot 4.7 API targeting succeeds.
- Full extension build/runtime equivalence is still an active R0 gate.

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

No production architecture is selected until the remaining bridge test and Android-device evidence are complete.
