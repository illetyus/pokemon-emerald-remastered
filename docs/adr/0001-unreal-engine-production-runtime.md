# ADR 0001 — Unreal Engine production runtime

- Status: Accepted
- Date: 2026-10-02

## Context

R0 compared several ways to preserve an authoritative Emerald/Vanilla+ gameplay core while replacing presentation and platform layers.

Observed results:

- Portable C core is deterministic and passes cross-platform regression tests.
- SDL3 + C is a valid low-level Android baseline and renders correctly on physical hardware.
- Godot + C works on desktop/headless CI, but repeated Android field tests exposed runtime-script/export friction unrelated to gameplay correctness.
- The production goal is not merely to reproduce GBA rendering. It is to build the highest-quality Android remaster while preserving Emerald gameplay behavior.

## Decision

The production presentation/runtime is **Unreal Engine 5.8**.

Architecture:

```text
Emerald / Vanilla+ gameplay data and rules
                  |
                  v
        Portable authoritative C core
                  |
                  v
          Unreal C++ adapter layer
                  |
        +---------+----------+
        |         |          |
        v         v          v
      World      UI        Battle
    rendering   UMG     presentation/VFX
        |
        v
      Android
```

The C gameplay core remains authoritative. Unreal is responsible for presentation and platform integration, not gameplay truth.

## Rules

1. Unreal presentation code must not directly mutate authoritative gameplay state.
2. Gameplay actions cross the adapter as explicit commands.
3. The core returns state snapshots and one-way presentation events.
4. Given the same initial state, data, random seed and action stream, Unreal must produce the same gameplay-state sequence as the portable core regression tests.
5. Android is the primary runtime target.
6. No production design should depend on GBA framebuffer, VRAM or palette constraints.
7. Existing Vanilla+ behavior is migrated progressively rather than rewritten from memory.

## Consequences

### Positive

- Direct C/C++ integration avoids a separate GDExtension-style runtime boundary.
- Much higher ceiling for 2.5D world presentation, lighting, materials, Niagara VFX, camera work and battle presentation.
- Unreal's Android packaging, profiling, device profiles and scalability systems become available.
- The same C core can continue to be tested outside Unreal.

### Costs

- Larger runtime and package footprint than the SDL3 baseline.
- Greater engine/build complexity.
- Pure 2D tile-map workflows are not the architectural center; the project will use a custom data-driven world presentation layer.
- Mobile performance budgets must be enforced from the beginning.

## R0 implication

Godot experiments are frozen as evidence. SDL3 remains the low-level baseline/reference implementation.

R0 now exits after the Unreal candidate proves:

- the shared C core compiles into the Unreal module;
- the canonical R0 scenario matches the core hash/event contract;
- an Android ARM64 development package launches on physical hardware;
- touch input reaches the core;
- pause/resume and persistent state survive Android lifecycle transitions.
