# R11 — Pokémon / Party / Item Core Completion Evidence

Date: 2026-10-05  
Branch: `remaster/r11-pokemon-party-item`  
Acceptance implementation head: `4246fd77e7ae04702d9c6a943cbb9fbdd3ce2223`

Dedicated R11 run: `37268875962`  
R0 regression run: `37268876075`  
R4 regression run: `37268876053`  
R5 regression run: `37268876061`  
R10 regression run: `37268875960`

## Status

**R11 COMPLETE at portable/source acceptance level.**

R11 establishes the authoritative Emerald Pokémon, party, PC storage and bag
binary/domain foundation required by R12 encounters, R13 battle, later UI and
R17 release-grade save compatibility.

No gameplay ownership moved into Unreal.

## Gen III Pokémon binary contract

R11 implements the real Generation III save representation rather than a new
remaster-owned Pokémon save model.

Accepted geometry:

- `BoxPokemon`: **80 bytes**;
- secure Pokémon payload: **48 bytes**;
- four canonical substructures: **4 × 12 bytes**;
- party Pokémon: **100 bytes**;
- party capacity: **6**;
- PC storage: **14 boxes × 30 Pokémon**.

The portable codec implements all **24** personality-dependent Gen III
substructure permutations.

Secure Pokémon data is XORed word-for-word with:

```text
personality ^ OT ID
```

The checksum is calculated over the complete plaintext 48-byte secure payload.

R11 preserves all four canonical 12-byte substructures, including bytes not yet
interpreted by current gameplay. This prevents future ribbon/origin/metadata
fields from being silently lost during a decode/encode round trip.

## Pokémon fields exposed by the portable core

The R11 API exposes or preserves:

- species;
- held item;
- experience;
- friendship;
- personality / nature;
- moves and PP;
- EVs;
- IVs;
- ability slot;
- met level;
- met game;
- Poké Ball;
- OT gender;
- full uninterpreted secure-substructure bytes.

Representative setters update the canonical plaintext state before the
80-byte structure is encoded.

## Party save geometry

The accepted Emerald SaveBlock1 layout is:

- party count: `0x0234`;
- party array: `0x0238`;
- six 100-byte party entries.

Party runtime fields include:

- status;
- level;
- mail;
- current HP;
- max HP;
- Attack;
- Defense;
- Speed;
- Sp. Attack;
- Sp. Defense.

The R11 regression verifies byte-for-byte writeback of a known encrypted
Pokémon plus party extension data.

## PC storage geometry

The accepted Pokémon-storage layout is:

- current box: storage offset `0x0000`;
- boxed-Pokémon payload begins at `0x0004`;
- 14 boxes;
- 30 entries per box;
- 80 bytes per entry;
- boxed-Pokémon payload ends exactly at `0x8344`, where box-name data begins.

This explicitly preserves the three-byte alignment gap after `currentBox`.

The regression exercises both the first and last legal PC slots.

## Bag / item save semantics

R11 uses the actual Emerald bag layout.

SaveBlock2:

- encryption key: `0x00AC`.

SaveBlock1 pockets:

- Items: `0x0560`, 30 slots;
- Key Items: `0x05D8`, 30 slots;
- Poké Balls: `0x0650`, 16 slots;
- TM/HM: `0x0690`, 64 slots;
- Berries: `0x0790`, 46 slots.

Bag quantities are stored as:

```text
quantity XOR low16(save encryption key)
```

Accepted slot limits:

- ordinary item slot: **99**;
- berry slot: **999**.

Vanilla slot behavior is preserved:

- Items / Key Items / Poké Balls may continue into another matching slot when
  the current slot reaches 99;
- TM/HM and Berry entries do not split one item across multiple slots;
- failed add operations do not partially mutate the bag;
- removing more than the owned quantity fails;
- an empty item slot reports quantity zero even when the save encryption key is
  non-zero.

## Deterministic source catalogs

R11 adds:

`tools/build_r11_domain_catalog.py`

It deterministically derives the portable domain tables from pinned
`vendor/vanillaplus` source data.

Checked-in generated output:

`core/src/emerald_domain_catalog.inc`

Audit report:

`data/r11/domain_catalog_report.json`

Accepted catalog counts:

- internal species slots: **412**;
- moves: **355**;
- items: **377**;
- evolution slots per species: **5**.

The test suite regenerates both outputs and compares them byte-for-byte to the
checked-in artifacts.

Representative source regressions include:

- Bulbasaur base stats/types/growth/Overgrow;
- Shedinja base HP = 1 and Wonder Guard;
- Treecko base stats and Medium Slow growth;
- Tackle = 35 power / Normal / 95 accuracy / 35 PP;
- Thunderbolt = 95 power / Electric / 100 accuracy / 15 PP;
- Potion = price 300 / Items pocket / effect parameter 20;
- Poké Ball = price 200 / Poké Ball pocket;
- pinned Vanilla+ Kadabra evolution = level 37 -> Alakazam;
- Feebas evolution = beauty 170 -> Milotic.

## EXP, nature and stat helpers

The portable domain implements Emerald growth curves:

- Medium Fast;
- Erratic;
- Fluctuating;
- Medium Slow;
- Fast;
- Slow.

Nature identity follows Gen III:

```text
personality % 25
```

The stat calculation follows the Gen III IV/EV/level formula and 10% nature
modifier.

The Shedinja HP=1 special case is regression-pinned.

## Unreal boundary

R11 adds portable Unreal embedding units for:

- `emerald_pokemon.c`;
- `emerald_items.c`.

The source architecture validator requires these embeds.

The existing C++17 embed smoke now directly calls:

- the R11 species catalog;
- the R11 bag core.

This is evidence that the portable C implementation remains valid through the
project's Unreal C++ embedding path.

It is **not** a claim that licensed Unreal Engine 5.8.3 was compiled or that an
APK was produced. Those remain deferred to R18.

## Regression evidence

Dedicated R11 run:

`37268875962`

Result: **SUCCESS**

The targeted CTest subset reported:

**6/6 tests passed.**

Passed R11 gates:

- Unreal C++ embed smoke;
- BoxPokemon encryption/permutation/checksum regression;
- party/storage geometry regression;
- bag encryption/capacity regression;
- species/move/item/evolution catalog regression;
- deterministic domain catalog audit;
- Unreal source architecture validation.

Earlier-phase regressions on the same implementation head:

- R0 Core `37268876075` — SUCCESS;
- R4 Overworld `37268876053` — SUCCESS;
- R5 World Renderer `37268876061` — SUCCESS;
- R10 Quest Map `37268875960` — SUCCESS.

## R11 exit checklist

- [x] 80-byte Gen III BoxPokemon codec;
- [x] 24 personality permutations;
- [x] secure XOR encryption/decryption;
- [x] full secure-payload checksum;
- [x] byte-preserving secure substructures;
- [x] 100-byte party Pokémon;
- [x] six-slot party layout;
- [x] 14×30 PC storage layout;
- [x] bag quantity encryption;
- [x] five canonical bag pockets;
- [x] Vanilla slot capacities/splitting behavior;
- [x] atomic failed bag add behavior;
- [x] 412 species slots;
- [x] 355 moves;
- [x] 377 items;
- [x] evolution data;
- [x] EXP/nature/stat helpers;
- [x] deterministic generator and audit;
- [x] C++17 Unreal embed smoke;
- [x] R0/R4/R5/R10 regressions green.

**R11 exit gate: PASSED.**

## Next phase

R12 — Encounter system.

R12 will consume this Pokémon-domain foundation while preserving R3/R4 world
authority and Emerald RNG call ordering.
