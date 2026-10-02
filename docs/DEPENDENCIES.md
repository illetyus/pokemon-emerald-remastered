# R0 Dependency Pins

R0 intentionally pins external toolchain dependencies. Architecture comparisons are not useful if each candidate silently moves to a different engine or API version.

## Godot

- Version: 4.7.2-stable
- Release date: 2026-08-18
- Linux x86_64 archive:
  `Godot_v4.7.2-stable_linux.x86_64.zip`
- SHA-256:
  `cadd3204e728a35d3f13adb7fd0d7902636b79f6b95c40c265eb73b6c35329e4`

R0 does not use Godot 4.8 development snapshots.

## godot-cpp

- Version: 10.0.0-stable
- Pinned commit:
  `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`
- Target API version: Godot 4.7

The bridge never tracks `master`.

## SDL

- Version: 3.4.16
- Source archive:
  `SDL3-3.4.16.tar.gz`
- SHA-256:
  `7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68`

## R0 state contract

Canonical scenario:
`shared/scenarios/r0_walk_event_encounter.json`

Canonical final C-state hash:
`0x642df4e66c5448a0`

Dependencies may only be changed during R0 with an explicit commit explaining why. Once R0 selects a production architecture, dependency updates move through normal compatibility testing.
