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
- map weather, weather-cycle stage, Flash state and saved-music load parity;
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
- [x] savedMusic / weather / weatherCycleStage / flashLevel state
- [x] Vanilla map-load weather-cycle translation
- [x] Vanilla outdoor Flash reset and dark-cave flash-level rules
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
- [x] real-save inspection/rewrite verifier CLI

## Validation status

Native R1 implementation is now corrected against both the pinned Vanilla+
source and a real VP019 save image. The real save exposed stale source-layout
comments: production SaveBlock2 is 0x0F44, SaveBlock1 is 0x3DC8, compiled
ObjectEvent stride is 0x28, object templates begin at 0x0CB0, flags at 0x12B0
and vars at 0x13DC. The compatibility review has re-checked save-sector
rotation/selection, ObjectEventTemplate layout,
warp-coordinate precedence, coord-event matching, background-event matching,
and map-connection coordinate rules.

Automated coverage is wired for all of those contracts, including the
deterministic save -> gameplay mutation -> save -> reload path.

Current evidence gates:

- [x] R1 implementation is present in the portable core
- [x] deterministic save/event/warp/object regression coverage is wired into CTest
- [x] the Unreal C++ embed preflight links and exercises the saved object-template layer
- [x] real-save inspection/rewrite verifier CLI exists and reopens its output
- [ ] portable C/C++ suite executes on a functioning runner after the latest commits
- [x] one real Vanilla+ VP019 128 KiB save is decoded with both slots/checksums valid
- [x] its Fiery Path map/object-template state is cross-checked against Vanilla+ source data
- [x] that save is rewritten by the native layer and re-decoded with byte-identical gameplay payloads
- [ ] the rewritten save is reopened successfully by the actual Vanilla+ ROM/emulator

The current GitHub Actions jobs are failing before their first workflow step
(no Checkout/Configure/Test step is created), so they do not constitute a
code-test failure. The portable suite remains an execution evidence gate until
a runner actually starts.

The full all-Hoenn conversion audit belongs to R3 in the fixed roadmap and is
not an R1 exit blocker. UE 5.8 Linux compile/cook/APK validation is the separate
parallel Unreal-build line and does not block the engine-independent R1-R4 core.

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


## Real save verifier

Once a real Vanilla+ 128 KiB save is available, inspect it without modifying the source file:

```text
./build/emerald_save_inspect path/to/input.sav
```

To exercise the native writer, always target a new file:

```text
./build/emerald_save_inspect path/to/input.sav --rewrite build/roundtrip.sav
```

The tool refuses to overwrite the input. It reopens the written file from disk, decodes it again and compares the authoritative SaveBlock1, SaveBlock2 and PokémonStorage payloads byte-for-byte. Save slot/counter metadata is allowed to advance as Emerald normally does.
