# Pokémon Emerald Remastered

Android-first technical remaster of Pokémon Emerald/Vanilla+.

The project preserves authoritative Emerald gameplay behavior in a portable,
deterministic C/C++ core while Unreal Engine owns presentation, UI, audio,
input and platform integration.

## Project status

**R0-R5, R10-R13, R16 and R17 are complete.**

R6/R7/R9 source preparation is verified on dependent branches. This R14 branch
adds the const battle presentation consumer, complete 386-species/form audit and
local Pokémon asset validation pipeline. Source/preparation acceptance is being
verified; zero modern 3D Pokémon imports are claimed. Real Unreal/UHT,
asset import and Android execution remain separate R18/R19 checks.

Completed foundation:

- R0 — production architecture / portable authoritative core;
- R1 — save, state, RTC and event foundation;
- R2 — script engine;
- R3 — complete Hoenn world-data conversion;
- R4 — authoritative overworld runtime;
- R5 — Unreal world-renderer source/data bridge;
- R10 — map + quest guidance derived from Emerald state;
- R11 — Pokémon / party / item core with Gen III save-compatible binary semantics;
- R12 — deterministic Emerald/Phase9 encounter core;
- R13 — portable deterministic Emerald/Vanilla+ battle core;
- R16 — regression-pinned portable Vanilla+ QoL policy and management core.
- R17 — source-format save compatibility, safe persistence and migration fixtures.

The next source/preparation phase after this branch's R14 acceptance is:

**R15 — Audio**

R14 evidence: [docs/R14_BATTLE_PRESENTATION_COMPLETION.md](docs/R14_BATTLE_PRESENTATION_COMPLETION.md).
R9 evidence: [docs/R9_UI_COMPLETION.md](docs/R9_UI_COMPLETION.md).
R6/R7/R9/R14 integration must retain dependency order; PR/merge/main gates remain pending.

The phase numbers are intentionally preserved even though the remaining
implementation order is non-numeric.

R13 completion evidence: **[docs/R13_BATTLE_CORE.md](docs/R13_BATTLE_CORE.md)**

R16 completion evidence: **[docs/R16_VANILLAPLUS_QOL.md](docs/R16_VANILLAPLUS_QOL.md)**

## Canonical roadmap

The current and authoritative development plan is:

**[docs/ROADMAP.md](docs/ROADMAP.md)**

Older R0-R5 plan/evidence documents are historical records. They do not define
the current remaining-phase order.

Current remaining order:

1. R17 — Save compatibility / migration
2. R6 — Character / NPC presentation + asset pipeline
3. R7 — Camera / environment presentation + asset pipeline
4. R9 — UI / HUD / menu infrastructure
5. R14 — Battle presentation + Pokémon asset pipeline
6. R15 — Audio
7. R8 — Android input infrastructure
8. R19 — Regression / test infrastructure expansion
9. R20 — Code / package polish
10. R18 — Real UE 5.8.3 / Android production build on the project PC
11. Real Unreal runtime validation
12. BrowserStack real Android smoke
13. R19 final Android device matrix
14. R21 — Release Candidate
15. R22 — Final Release

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
