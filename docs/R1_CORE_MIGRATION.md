# R1 — Vanilla+ Save / Overworld State Migration

Production gameplay behavior is sourced from:

- repository: `illetyus/pokezumrut-vanillaplus`
- pinned baseline: `70db90c9077aed1272e746fc2537d9f12b95a91c`

Native-platform reference:

- `gradenGnostic/pokeemerald-multiplatform`

The multiplatform project is a reference for native platform separation. Vanilla+ remains the authoritative gameplay/content source.

## R1 boundary

R1 establishes the native compatibility boundary needed before the full script engine and later gameplay systems are migrated.

In scope:

- Emerald 128 KiB save-sector format;
- slot/checksum/counter/rotation handling;
- platform save I/O;
- RTC offset and reconciliation;
- SaveBlock1 overworld location/state;
- encrypted money/coins;
- registered item and party-count metadata;
- persistent and temporary flags/vars;
- continue-game, dynamic, last-heal and escape warp state;
- map group/map number/layout identity;
- numeric object/warp/coord/background-event identities;
- Vanilla object visibility from hide flags;
- coord-event var matching;
- background sign/hidden-item/secret-base resolution;
- warp application and map-entry temporary-state reset;
- deterministic save -> gameplay mutation -> save -> reload regression;
- Unreal typed bridge for legacy save/world state.

Out of scope for R1:

- full Emerald script-bytecode migration;
- complete party/Pokémon interpretation;
- bag/item/PC domain;
- encounter RNG;
- battle engine;
- quest logic;
- final overworld movement loop;
- presentation assets.

Those continue in later phases.

## Implemented

- [x] 128 KiB Emerald save decoder/encoder
- [x] 14-sector dual-slot validation
- [x] checksum/signature/counter handling
- [x] damaged-slot fallback
- [x] preservation of non-main save sectors
- [x] platform save callbacks
- [x] RTC local offset and last-berry-update fields
- [x] SaveBlock1 player/map/warp/layout state
- [x] encrypted money and coins
- [x] registered item and party count
- [x] persistent flag/var access
- [x] temporary flag/var map-load reset
- [x] continue-game warp
- [x] dynamic warp
- [x] last-heal warp
- [x] escape warp
- [x] numeric map group/map identities
- [x] numeric flag/var/warp event identities
- [x] Vanilla object-event local IDs
- [x] object hide-flag resolver
- [x] coord trigger resolver
- [x] weather coord resolver
- [x] sign/hidden-item/secret-base resolver
- [x] warp transition state
- [x] Unreal legacy-save subsystem
- [x] Unreal world-gameplay subsystem
- [x] synthetic R1 end-to-end save/gameplay regression

## Remaining gates

- [ ] portable C/C++ test suite executes successfully after the latest R1 commits
- [ ] full Vanilla+ world conversion audit succeeds on the pinned source tree
- [ ] at least one real Vanilla+ `.sav` is decoded and compared against known in-game state
- [ ] the same real save is rewritten by the native layer and reopened successfully
- [ ] Unreal Engine 5.8 build validates the typed R1 bridge

The first two are automated/code gates. The real-save gate requires a known Vanilla+ save fixture. The Unreal build gate requires the UE 5.8 Linux build environment.

## Regression commands

Portable build:

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

World conversion:

```text
python tools/convert_world.py <pokezumrut-vanillaplus-root> build/generated-world
python tools/audit_generated_content.py build/generated-world
```

Platform-coupling inventory:

```text
python tools/scan_platform_coupling.py <pokezumrut-vanillaplus-root> --output build/platform-coupling.json
```

## Exit criterion

R1 closes when the native layer can take an existing Vanilla+ save, reproduce its persistent overworld/map/event state, mutate that state through native APIs, write a valid Emerald-compatible save image, and load the result again without GBA hardware dependencies.
