# Pokémon Emerald Remastered

Android-first technical remaster of Pokémon Emerald/Vanilla+.

The project preserves authoritative Emerald gameplay behavior in a portable,
deterministic C/C++ core while Unreal Engine owns presentation, UI, audio,
input and platform integration.

## Project status

**R0-R5 and R10-R11 are complete.**

Completed foundation:

- R0 — production architecture / portable authoritative core;
- R1 — save, state, RTC and event foundation;
- R2 — script engine;
- R3 — complete Hoenn world-data conversion;
- R4 — authoritative overworld runtime;
- R5 — Unreal world-renderer source/data bridge;
- R10 — map + quest guidance derived from Emerald state;
- R11 — Pokémon / party / item core with Gen III save-compatible binary semantics.

The next implementation phase is:

**R12 — Encounter system**

The phase numbers are intentionally preserved even though the remaining
implementation order is non-numeric.

## Canonical roadmap

The current and authoritative development plan is:

**[docs/ROADMAP.md](docs/ROADMAP.md)**

Older R0-R5 plan/evidence documents are historical records. They do not define
the current remaining-phase order.

Current remaining order:

1. R12 — Encounter system
2. R13 — Battle core
3. R16 — Vanilla+ QoL
4. R17 — Save compatibility / migration
5. R6 — Character / NPC presentation + asset pipeline
6. R7 — Camera / environment presentation + asset pipeline
7. R9 — UI / HUD / menu infrastructure
8. R14 — Battle presentation + Pokémon asset pipeline
9. R15 — Audio
10. R8 — Android input infrastructure
11. R19 — Regression / test infrastructure expansion
12. R20 — Code / package polish
13. R18 — Real UE 5.8.3 / Android production build on the project PC
14. Real Unreal runtime validation
15. BrowserStack real Android smoke
16. R19 final Android device matrix
17. R21 — Release Candidate
18. R22 — Final Release

## Core principles

- Portable core owns gameplay truth.
- Unreal is presentation/input/audio/platform host only.
- Emerald/Vanilla+ source behavior is the primary gameplay specification.
- Same initial state + data + RNG seed + ordered inputs must produce the same
  gameplay-state sequence.
- Regression-first development.
- Each phase uses branch + PR and merges only after required CI is green.
- No direct commits to `main`.
- Android is the primary runtime target.
- Real Unreal compilation, APK production and BrowserStack real-device testing
  are deliberately deferred until the final PC stage.
- Do not commit ROMs, extracted Nintendo/Game Freak/Creatures commercial
  assets, credentials or secrets.

## Architecture

See:

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- [docs/adr/0001-unreal-engine-production-runtime.md](docs/adr/0001-unreal-engine-production-runtime.md)

Production architecture:

```text
Emerald / pinned Vanilla+ source truth
        ↓
portable deterministic gameplay core
        ↓
state snapshots + presentation events
        ↓
Unreal Engine
        ↓
Android
```

## Asset policy

Character, Pokémon, environment and audio preparation use a local asset
pipeline.

The public repository may contain:

- extraction/conversion tools;
- mappings;
- manifests;
- hashes/provenance;
- validation rules;
- redistributable test/placeholder assets.

It must not contain extracted commercial game models, textures, audio or ROM
images.

## Unreal / Android build policy

Source/data architecture checks continue in ordinary CI.

The real licensed Unreal Engine 5.8.3 compile/cook/package step is intentionally
scheduled for R18 on the project PC. A real Android APK is required before the
BrowserStack real-device smoke stage.

A future self-hosted runner, if used at all, must never execute untrusted public
fork PR code with host or secret access.
