# Clean source build and real-engine prerequisites

## Public portable/source acceptance

Start from a complete clean Git checkout of the selected branch/commit, including
the tracked vendor snapshot. Partial source caches cannot reproduce the full
suite. The source pin is Vanilla+ 70db90c9077aed1272e746fc2537d9f12b95a91c;
accepted vendored tree is 5a551f1f9e40184278c57dfb8d25f68a0a1c99dc.
Do not fetch a moving upstream or modify vendor to make a check pass.

Install Git, Python >=3.10, C99/C++17 compilers and CMake/CTest >=3.21.
FFmpeg and ffprobe must be on PATH for the required transform tests.
[DEPENDENCIES.md](DEPENDENCIES.md) separates production tools and historical pins.

From the repository root:

```sh
python tools/r19_full_suite.py --build-dir build --receipt build/r19-full-suite-receipt.json
```

This configures/builds portable targets, executes discovered CTest and Python
regressions with mandatory coverage/zero skips, audits assets, repeated packages,
source smoke preparation, device-matrix metadata and Unreal architecture.
Failure is fatal. Receipts contain actual counts; they do not certify UE/device
execution. Hosted R19 installs FFmpeg and runs this same entry point.

## Production source/data package

Use fresh destinations, with receipts outside the package:

```sh
python tools/build_production_package.py --output build/production/Generated --receipt build/r20-generation-receipt.json
python tools/build_production_package.py --output build/production/Generated --verify --expected-index-sha256 <hash-from-trusted-generation-receipt> --receipt build/r20-verification-receipt.json
```

Read index_sha256 from the trusted generation receipt. Do not derive trust by
hashing an untrusted received index. Generation checks the clean pinned vendor,
518 world maps / 441 layouts, 75 render tilesets, complete character/environment
catalogs and native R9/R14/R15 generated catalogs. Four owner directories retain
their formats. All files and conservative source inputs are sealed before
transactional publication. [Package policy](R20_PRODUCTION_PACKAGE.md) defines
output isolation, unsafe-path rejection and verification.

Current R20 work separately closes ignored private/generated outputs, security,
Unreal staging, preflight and clean-repeat proof. Until their named checkpoints
pass, generation success alone does not close those obligations.

## Private optional presentation and save inputs

The public commands above require no ROM, private save, commercial model/audio,
private archive, account or secret. A private real save may replay the R17
compatibility matrix locally; synthetic versioned fixtures remain public.
Never commit that save or infer a new persistent Emerald layout from UI needs.

| Owner | Optional local work and authoritative instructions |
| --- | --- |
| R6 characters | Source archives, normalized geometry/materials/clips, exact import bindings; [completion and fallback limits](R6_PRESENTATION_COMPLETION.md) |
| R7 environment | Identity-specific normalized fragments, LODs/textures, hashes and import readiness; [completion](R7_ENVIRONMENT_COMPLETION.md) |
| R14 Pokémon | Exact species/form/shiny source bindings, scale/clips and editor validation; [completion](R14_BATTLE_PRESENTATION_COMPLETION.md) |
| R15 audio | Private normal/special cry packs, selected BGM renderer/bank inputs, provenance, listening/loop/import validation; [completion](R15_AUDIO_COMPLETION.md), [pack recovery](R15_AUDIO_PACK_RECOVERY.md), [BGM coverage](R15_BGM_RENDER_COVERAGE.md) |

Defaults preserve explicit visible/missing-asset fallbacks. Zero imported modern
Pokémon models and unverified actual audio/runtime quality remain explicit;
selected recipes or private candidates are not cooked game content.
Use ignored local directories; no private payload may become a public artifact.

## Real engine/build stage

R18-P1 requires actual project-PC control, sufficient OS/storage and the licensed
UE/Android environment. If unavailable, stop EXTERNAL_ENV_REQUIRED.
R18-P2 verifies exactly UE 5.8.3 from installation/build metadata; R18-P3 matches
SDK/NDK/JDK/UBT to that build. Do not guess compatible toolchain versions.
The existing package identity/SDK settings are source intent pending this audit.

Only actual target compilation, cook, Android ARM64 package and APK integrity
evidence close R18. Release source configuration/preflight does not execute those
steps. Existing R0GameMode and RemasterCoreSubsystem remain live bootstrap;
historical UI moved outside the runtime module is not a runtime replacement.

Story/script resource hosts, battle request/finalization owners and naming/
field-item editors retain the explicit R8/R9/R14 host obligations. Actual imports,
rendered correctness, performance, audio listening, Android lifecycle and physical
input behavior need real runtime checks. After verified R18 main CI, stop at
REAL UNREAL RUNTIME VALIDATION. Physical Android, BrowserStack, final device
matrix, R21/R22 do not run automatically.

Source release/install preflight and staged owner directories:
[R20_RELEASE_PREPARATION.md](R20_RELEASE_PREPARATION.md). Complete tracked-source/
history leakage and credential-signature audit:
[R20_PUBLIC_SOURCE_SECURITY.md](R20_PUBLIC_SOURCE_SECURITY.md).
