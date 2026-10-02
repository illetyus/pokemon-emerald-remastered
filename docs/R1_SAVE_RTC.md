# R1 Save / RTC Compatibility

Status: implementation complete, engine/device validation pending.

Authoritative source baseline:

- `illetyus/pokezumrut-vanillaplus`
- commit `70db90c9077aed1272e746fc2537d9f12b95a91c`

## Preserved save contracts

The portable layer preserves the existing Emerald / Vanilla+ 128 KiB flash image.

Main-save geometry:

- 32 total 4 KiB sectors;
- 14 sectors per alternating main slot;
- sectors 28-31 remain outside normal main-save rotation;
- 3968 bytes of data per main sector;
- footer fields remain sector id, checksum, signature and counter;
- signature remains `0x08012025`.

Persistent block sizes are pinned to the Vanilla+ source layout:

- SaveBlock2: `0x0F2C`
- SaveBlock1: `0x3D88`
- PokemonStorage: `0x83D0`

The new implementation reconstructs logical SaveBlock2, SaveBlock1 and PokemonStorage from rotated sectors, selects the newest valid slot, falls back to the older valid slot when the newer copy is damaged, and writes the next save using Emerald's counter/rotation scheme.

It does not convert the user's save to a new application-specific format.

## RTC

Vanilla+ persists RTC state in the existing SaveBlock2 fields:

- `localTimeOffset` at `0x98`
- `lastBerryTreeUpdate` at `0xA0`

The portable RTC layer preserves Emerald's original semantics:

1. obtain platform wall clock;
2. convert the full year back to the GBA RTC two-digit-year representation;
3. calculate Emerald day count;
4. subtract persisted `localTimeOffset`;
5. borrow seconds/minutes/hours exactly as the original code does.

RTC correction updates the existing offset and `lastBerryTreeUpdate`; no new persistent RTC fields are introduced.

## State view

The portable save-state view currently exposes:

- player tile coordinates;
- current map group / map number / warp id;
- warp coordinates;
- map layout id;
- weather / flash level;
- party count;
- encrypted money and coins using the existing SaveBlock2 encryption key;
- registered item;
- persistent flags;
- persistent vars;
- current local RTC time.

Persistent flag bytes remain at SaveBlock1 `0x1270`.

Persistent vars remain at SaveBlock1 `0x139C`, with IDs beginning at `0x4000`.

## Unreal bridge

`URemasterVanillaPlusSaveSubsystem` owns the engine-side bridge.

Its canonical compatibility file is:

```text
Saved/Core/vanillaplus.sav
```

The subsystem depends on the core platform subsystem, loads the 128 KiB file through the portable platform vtable and exposes state/flag/var/RTC queries to C++ and Blueprint.

Unreal does not parse Emerald flash sectors itself.

## Regression coverage

Portable tests cover:

- checksum calculation;
- two valid save slots;
- newest-counter selection;
- rotated sector reconstruction;
- fallback when newest slot is damaged;
- SaveBlock1/2 and PokemonStorage tail boundaries;
- RTC persisted offsets;
- next-slot write/reload;
- preservation of sectors outside the main-save slots;
- platform-backed load/store;
- map/player state view;
- encrypted currency;
- flag set/clear;
- var get/set;
- RTC realignment and wall-clock progression.

## Remaining validation

R1 Save/RTC is not marked field-validated until:

- the portable test suite runs successfully on CI again;
- Unreal 5.8 compiles the bridge;
- a real existing Vanilla+ 128 KiB save is loaded;
- the same map/player/party state is observed;
- save/reload succeeds on Android without changing the legacy layout.
