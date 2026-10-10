# Pokémon Emerald Remastered

Android-first remaster with a portable deterministic C/C++ gameplay core and
Unreal presentation, input, audio and platform integration.

## Current state

R0–R5, R10–R13, R16/R17 and the pre-real-Unreal source phases
R6/R7/R9/R14/R15/R8/R19 are verified and merged on main. R20 production polish
is active on `r20-production-polish` / PR #24. R19 main acceptance at
`ab057754f97fc1d8f5e9396ff8499337bb533dc3` passed 81 CTest checks,
400 Python tests and 171 asset audits; all 16 exact-main workflows succeeded.

These are portable/source results. Real UE/UHT compile, imported modern assets,
cook, APK, rendered gameplay and Android/device quality remain unverified.
Source fallbacks and unconnected host obligations are recorded in phase evidence.

Remaining automatic order: **R20 → R18 → STOP** at
**REAL UNREAL RUNTIME VALIDATION**. If actual project-PC/UE/Android access is
unavailable, stop at the corresponding R18 checkpoint as EXTERNAL_ENV_REQUIRED.
Physical Android smoke, BrowserStack, final device matrix, R21 and R22 require
the later runtime stage and do not start automatically.

## Plan and build

- [Canonical roadmap](docs/ROADMAP.md)
- [Named execution checkpoints and current resume point](docs/PHASE_EXECUTION_PLAN.md)
- [Architecture and gameplay authority](docs/ARCHITECTURE.md)
- [Clean build, generation and private prerequisites](docs/BUILD.md)
- [Public dependencies and preserved historical pins](docs/DEPENDENCIES.md)
- [Production package layout and trusted verification](docs/R20_PRODUCTION_PACKAGE.md)
- [Unreal build boundary](unreal/README.md)

From a complete clean checkout with the documented public tools:

```sh
python tools/r19_full_suite.py --build-dir build --receipt build/r19-full-suite-receipt.json
python tools/build_production_package.py --output build/production/Generated --receipt build/r20-generation-receipt.json
```

The first command runs actual portable/source regressions. The second delegates
to the unchanged world/render/character/environment owners and seals their bytes
with source provenance. Neither command runs Unreal or produces an APK.

## Development rules

The pinned Emerald/Vanilla+ production source defines gameplay. Same initial
state, source data, RNG seed and ordered inputs must produce the same state
sequence. Unreal consumes snapshots/events and submits explicit commands;
movement, collision, scripts, progression, Pokémon/items and battles stay in core.

Use a phase branch and PR, regression-first changes, terminal-success final CI
before merge and exact-main CI before the next phase. Do not commit directly
to main, edit the vendor snapshot, or use workflows to self-commit implementation.

Public source contains converters, mappings, manifests, hashes, validators and
redistributable fixtures. ROMs, extracted commercial models/textures/audio,
private saves, APKs, credentials and secrets stay outside public tracking.
SDL/Godot and the removed R0 presentation pair remain historical references.
