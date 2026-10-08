# Unreal Production Runtime

Unreal Engine is the selected production presentation/platform runtime.

Gameplay authority remains in the portable core.

## Engine target

Planned production engine:

- Unreal Engine 5.8.3;
- Android-first runtime target;
- ARM64;
- real compile/cook/package deferred to R18 on the project PC.

Android SDK/NDK/JDK requirements must be re-verified against the exact installed
UE 5.8.3 build at R18 before toolchain setup.

## Current status

R5 source/data world-renderer acceptance is complete.

Before R18, the repository may validate:

- Unreal source architecture;
- portable core behavior;
- converted world/render data;
- generated manifests and package integrity.

Before R18, the project does **not** claim:

- successful licensed UE compilation;
- rendered-frame runtime acceptance;
- cooked Android package acceptance;
- APK launch success;
- BrowserStack real-device success.

## Architecture

Unreal is a presentation/input/audio/platform host.

It may:

- render authoritative world state;
- present characters and Pokémon;
- animate;
- control camera and lighting;
- show UI;
- play audio;
- collect platform input;
- provide Android lifecycle/platform services.

It must not independently own:

- movement rules;
- gameplay collision;
- scripts;
- flags/vars;
- warps/connections;
- encounter decisions;
- Pokémon/party/item truth;
- battle results;
- story progression;
- save-domain gameplay truth.

Input crosses the adapter as explicit actions/commands. The core returns state
snapshots and one-way presentation events.

## Asset pipeline

Commercial character, Pokémon, environment and audio assets are prepared
locally from user-owned source data where applicable.

The public repository stores only tooling and redistributable metadata such as:

- converters;
- mappings;
- manifests;
- hashes/provenance;
- validators;
- placeholders/test assets with appropriate redistribution rights.

Extracted commercial models, textures, audio and ROM images must not be
committed.

## Deferred R18 build stage

At R18, on the project PC:

1. install/verify Unreal Engine 5.8.3;
2. install/verify the Android toolchain required by that UE build;
3. compile the production Unreal target;
4. cook/package Android ARM64;
5. produce an installable APK;
6. run local physical-device smoke;
7. fix compile/cook/runtime problems through normal branch/PR workflow.

Only after a valid APK exists do BrowserStack real-device tests begin.

## Presentation phases awaiting real runtime validation

The following phases may be source/data complete before R18, but their final
runtime gates wait for the real engine build:

- R6 — character/NPC presentation;
- R7 — environment/camera presentation;
- R8 — Android input;
- R9 — UI/HUD/menus;
- R14 — battle presentation;
- R15 — audio.

## Self-hosted runner policy

A self-hosted Unreal runner is **not required now**.

If one is introduced later, it must not execute untrusted public-fork pull
request code with access to the host, credentials or secrets.

The default final plan is to perform the first real production UE/Android build
on the project PC.

## Historical R0 experiments

SDL3 remains a low-level regression/reference baseline.

Godot experiments are frozen as historical architecture evidence.

Neither is the production presentation runtime.

See the canonical plan:

- `docs/ROADMAP.md`
- `docs/ARCHITECTURE.md`

## R6 character presentation

Generate and verify the character catalog before cook:

```sh
python tools/build_r6_character_package.py unreal/Content/Generated/Characters
python tools/build_r6_character_package.py unreal/Content/Generated/Characters --verify
```

The default runtime uses visible engine basic shapes. Optional exact local skeletal
assets require matching normalized-output hashes and validated import bindings;
no private account/network is needed. NPC transforms/visibility come from R4.
See [R6 presentation preparation and limits](../docs/R6_PRESENTATION_COMPLETION.md).
Real UE/UHT, cooked imports and Android device validation remain R18 work.

## R7 environment and camera presentation

Generate and verify the metadata-only world identity/audit package before cook:

```sh
python tools/build_r7_environment_package.py vendor/vanillaplus unreal/Content/Generated/Environment
python tools/build_r7_environment_package.py vendor/vanillaplus unreal/Content/Generated/Environment --verify
```

Camera position, map framing, runtime weather and local RTC are read from the
authoritative core context. R5 terrain is the default fallback for all identities;
optional private 3D fragments are exact-identity, import-validated bindings only.
They never contribute collision, overlaps, navigation or movement. Source budgets,
render-only camera cutaway, local pre-import validation and real-engine limits are
recorded in [R7 completion evidence](../docs/R7_ENVIRONMENT_COMPLETION.md).
