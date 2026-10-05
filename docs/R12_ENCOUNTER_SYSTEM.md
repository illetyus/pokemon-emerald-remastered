# R12 — Encounter System Completion Evidence

Date: 2026-10-05  
Branch: `remaster/r12-encounter-system`  
Acceptance head before documentation: `ceef979f9ece8713adc3014e9b9f2beb5893f93f`

## Status

**R12 COMPLETE at portable-core/source-host acceptance level.**

R12 ports the Emerald wild-encounter decision path into the portable gameplay
core while preserving the previously accepted Vanilla+ Phase 9 ecosystem
behavior.

The ownership model is:

```text
R4 authoritative movement/map state
        ↓
R12 encounter trigger/rate/RNG
        ↓
Phase9 local ecosystem/species bag
        ↓
R11 Pokémon construction
        ↓
portable encounter result
        ↓
Unreal presentation event only
```

Unreal does not choose species, level, nature, IVs, encounter rates or RNG
outcomes.

## Source truth

R12 is generated/audited from the pinned `vendor/vanillaplus` source.

Primary source inputs include:

- `src/wild_encounter.c`;
- `src/phase9_wild_ecosystem.c`;
- `src/data/phase9_wild_ecology.h`;
- `src/data/wild_encounters.json`;
- `src/metatile_behavior.c`;
- `src/roamer.c`;
- Pokémon level-up learnsets;
- map/species/ability/item/flag/var/weather constants.

No second hand-maintained numeric encounter table is authoritative.

## Generated encounter catalog

`tools/build_r12_encounter_catalog.py` deterministically derives:

- 124 encounter maps;
- 95 land maps;
- 55 water maps;
- 6 Rock Smash maps;
- 53 fishing maps;
- 386 National Dex habitat assignments;
- 412 internal-species learnset slices;
- metatile encounter/surf/bridge classifications;
- Phase 9 habitat overrides;
- local guaranteed species rules;
- roamer route-location data;
- pinned numeric flag/var/ability/item/weather identities.

The generator is executed during CMake configuration and is independently
rerun by the R12 Python audit.

## Emerald RNG

R12 implements the Emerald LCRNG:

```text
state = 1103515245 * state + 24691
Random() = state >> 16
```

The runtime stores both RNG state and call count.

Regression coverage pins:

- exact fixed-seed output sequence;
- `Random32` low/high call ordering;
- number of consumed RNG calls;
- fixed-seed end-to-end encounter replay.

## Encounter timing

R12 reproduces the relevant step order:

1. successful R4 movement completes;
2. repel counter is updated;
3. if repel expires, the repel event consumes that step before wild RNG;
4. Emerald's four-step wild-encounter immunity gate is applied;
5. metatile transition eligibility is checked;
6. encounter-rate modifiers are applied;
7. encounter RNG is rolled;
8. special encounter priority is checked;
9. the Phase 9 local species/level path runs;
10. the complete wild Pokémon is constructed through R11.

Blocked movement, warp and connection resolution do not fabricate an encounter.

## Encounter-rate rules

Regression coverage includes:

- base rate ×16;
- bicycle 80% modifier;
- encounter-up/down flutes;
- Cleanse Tag;
- Stench;
- Illuminate;
- White Smoke;
- Arena Trap;
- Sand Veil during sandstorm;
- the Emerald rate clamp at 2880.

The modifier implementation is portable-core-owned.

## Repel

R12 uses the existing Emerald repel variable and does not create a new persisted
repel field.

Behavior covered by tests:

- first living, non-egg party Pokémon supplies the repel level;
- wild level below that level is blocked;
- equal/higher level is allowed;
- counter decrements in Emerald step order;
- the expiration step returns an explicit `repel_wore_off` result;
- Union Room / Battle Pike / Battle Pyramid exclusions are preserved by the
  portable contract.

## Phase 9 ecosystem

The accepted Vanilla+ ecosystem remains authoritative for species and level.

R12 preserves:

- 386-species habitat coverage;
- land/water/fishing/Rock Smash habitat inference;
- Phase 9 map habitat overrides;
- deterministic local-pool hashing;
- maximum local pool size of 64;
- source anchor species preservation;
- New Mauville Mr. Mime guarantee;
- shuffled species-bag selection;
- no duplicate species inside one bag cycle;
- no immediate repeat across a bag boundary where alternatives exist.

The old stock Route 119 Feebas-spot selector is not silently re-enabled;
current pinned Vanilla+ fishing behavior remains the specification.

## Level selection

Wild level uses the average level of living, non-egg party Pokémon.

Accepted rule:

```text
average party level ± 15
minimum 2
maximum 100
```

An empty eligible party falls back to level 2 without consuming level-selection
RNG.

## Wild Pokémon construction

R12 constructs a battle-ready R11 Pokémon rather than returning only a species
number.

The portable result includes:

- species;
- level;
- personality;
- nature;
- gender;
- ability slot;
- six IVs;
- experience appropriate for the level/growth rate;
- friendship;
- Emerald origin metadata;
- initial level-up moves and PP;
- calculated party stats;
- RNG call range used to produce the result.

Cute Charm and Synchronize are evaluated in the portable creation path.

## Special encounters

Portable R12 paths cover:

- normal land/water encounter;
- mass outbreak priority;
- roaming Pokémon priority/state restoration;
- fishing;
- Rock Smash;
- Battle Pike/Pyramid explicit encounter-kind hooks;
- Sootopolis story-state water encounter suppression.

Outbreak species/level/moves come from existing Emerald save fields.

Roamer persistent Pokémon data comes from the existing save, while its
transient map location belongs to the R12 runtime, matching the original
persistent-vs-EWRAM split.

## R4 / Unreal integration

`URemasterWorldGameplaySubsystem` owns opaque storage for
`RemasterEmeraldEncounterRuntime`.

After a successful ordinary R4 movement step it:

- reads the authoritative map behavior;
- passes map/tile/save context into `remaster_emerald_encounter_step()`;
- receives the portable result;
- exposes presentation-only species/level/nature/gender/ability data;
- emits `OnWildEncounterGenerated` when an encounter occurs;
- exposes repel expiration as a presentation event field.

Unreal contains no species pool, encounter-rate formula or encounter RNG.

### Deferred avatar-state boundary

R4 deliberately deferred bike- and surf-specific avatar movement state.

Therefore:

- R12 core fully implements the bicycle encounter-rate modifier;
- R12 core fully supports the `surfing` context required for
  bridge-over-water behavior;
- ordinary water encounter tiles work from authoritative map behavior;
- the Unreal R4 host currently supplies `biking = 0` and `surfing = 0`
  until authoritative bike/surf avatar state is introduced by the later
  movement/input work.

This is an explicit integration boundary, not presentation-owned inference.

## Regression evidence

Dedicated R12 run on the accepted implementation head:

- R12 Encounter `37285323370` — SUCCESS.

The R12 CTest subset reported:

- Unreal C++ embed — PASS;
- R12 RNG — PASS;
- R12 modifier/repel rules — PASS;
- R12 Phase 9 ecosystem — PASS;
- R12 special encounters — PASS;
- R12 fixed-seed replay — PASS.

Result:

**6/6 tests passed.**

The Python source audit additionally passed:

- deterministic generator;
- source coverage / Phase 9 contract;
- current Phase 9 fishing / legacy Feebas behavior guard.

Same-head earlier-phase regressions:

- R0 Core `37285323307` — SUCCESS;
- R4 Overworld `37285323246` — SUCCESS;
- R5 World Renderer `37285323667` — SUCCESS;
- R10 Quest Map `37285323296` — SUCCESS;
- R11 Pokémon / Party / Item `37285323333` — SUCCESS.

## CodeQL

The initial PR scan raised one high-severity CodeQL alert for an inefficient
regular expression in the Python catalog generator.

That regex was replaced. The GitHub Advanced Security review thread is now
resolved/outdated, and the current-head language analyzers report success for:

- actions;
- C/C++;
- C#;
- Python.

The final documentation head is re-scanned before merge.

## R12 exit checklist

- [x] Emerald RNG + call ordering;
- [x] fixed-seed replay;
- [x] 124-map source catalog;
- [x] 386-species Phase 9 ecology;
- [x] local species bag and repeat protection;
- [x] party-average ±15 level selection;
- [x] encounter-rate modifiers;
- [x] repel ordering and expiry result;
- [x] four-step wild immunity;
- [x] metatile land/water/bridge classification;
- [x] deterministic wild Pokémon construction;
- [x] initial level-up moves;
- [x] Synchronize / Cute Charm creation effects;
- [x] outbreak;
- [x] roamer;
- [x] fishing;
- [x] Rock Smash;
- [x] Sootopolis story suppression;
- [x] Unreal presentation-only adapter;
- [x] R0/R4/R5/R10/R11 regression gates green;
- [x] no new persisted encounter state in Emerald save geometry.

**R12 exit gate: PASSED.**

## Next phase

R13 — Battle Core.

R13 consumes R12's complete generated wild Pokémon and deterministic encounter
result; it owns battle state, move resolution, damage, status, abilities,
switching, fainting, experience and battle-end state.
