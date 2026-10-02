# Architecture

## Goal

Preserve authoritative Pokémon Emerald gameplay behavior while allowing rendering, UI, input, audio and platform integration to be replaced independently.

## Layers

### Gameplay core

Owns gameplay truth only:

- map position and collision result
- events and flags
- encounters
- quests and progression
- Pokémon, moves, items and trainers
- battle rules and deterministic results
- save-domain state

The gameplay core must not depend on Android UI, Godot scene objects, SDL windows, OpenGL/Vulkan objects or touch coordinates.

### Presentation

Consumes immutable or explicitly exported gameplay state and renders it.

Examples:

- overworld visuals
- animation
- camera
- lighting
- particles
- battle presentation
- UI
- audio cues

Presentation may request gameplay actions through the input API. It must not mutate gameplay state directly.

### Platform

Provides services whose implementation changes by target:

- input devices
- clock/RTC
- filesystem and saves
- app lifecycle
- window/display
- audio device
- haptics

## Core invariant

Given the same initial state, data set, random seed and ordered gameplay inputs, all front ends must produce the same gameplay state sequence.

This is the primary regression contract for the remaster.

## R0 boundary

R0 uses synthetic data only. It exists to test architecture and tooling without coupling the prototype to copyrighted commercial game assets.

The production migration of Vanilla+ gameplay is intentionally deferred until the winning architecture is selected.

## Candidate architectures

1. Portable C gameplay core + SDL3/OpenGL front end.
2. Portable C gameplay core + Godot presentation bridge.
3. Godot reimplementation of gameplay and presentation.

All three candidates must consume the same R0 scenario and report comparable results.

## Directional preference

The current hypothesis is candidate 2: portable C gameplay core with a Godot presentation layer. This is not an architectural decision until R0 evidence is complete.
