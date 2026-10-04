# Vanilla+ Phase 9 — Ecological Wild Ecosystem

Date: 2026-10-02
Branch: `phase9-wild-ecosystem`
Version owner: `VP017 / T017 / V+017`

## Goal

Replace Emerald's repetition-heavy weighted wild slots with a discovery-oriented ecosystem in which all 386 National Dex species can appear in physically appropriate habitats, eligible species have equal base probability, evolved forms are independent species, and levels scale around the player's living party.

## Runtime model

`vanilla map fauna -> dominant habitat profile -> deterministic route-local pool -> shuffled equal-probability bag -> dynamic level`

The original encounter tables remain as ecological seeds and still provide encounter-rate/map-presence information. Their distinct species are always retained once in the local pool, so they preserve a route's ecological identity without retaining vanilla weighting. Phase 9 does not enlarge SaveBlock or PokemonStorage.

## Habitat model

Generation III Pokedex habitat classes are used as the baseline:

- cave
- forest
- grassland
- mountain
- rough terrain
- sea
- urban
- waters-edge

The canonical Pokedex `rare` bucket is not used as a rarity weight. Its ten species are mapped to physical habitats so they participate under the same probability rule.

For each vanilla encounter slice, duplicate species are collapsed before habitat inference. The dominant habitat is selected, with a second habitat allowed when it has at least half the distinct-species support of the dominant habitat. This preserves mixed environments without allowing one unusual vanilla species to import an unrelated entire fauna.

Broad Generation III habitat categories are still too large to use directly. Phase 9 therefore deterministically partitions habitat-compatible species by map, encounter method and rod. The partition is stable rather than rerolled at runtime: revisiting the same route exposes the same local ecosystem, while neighboring routes can have meaningfully different fauna. Original vanilla species for a map/method are retained as ecological anchors, but receive no extra probability.

The partition density adapts to the size of the broad habitat candidate set. Current verification yields local pools in the approximate 11–36 species range, with a hard runtime ceiling of 64 entries. A small set of land/rock micro-biome overrides corrects obvious coarse-habitat errors for the volcanic belt, Mt. Pyre, Shoal Cave, New Mauville, desert areas and Seafloor Cavern.

Fishing infers habitat separately for Old, Good and Super Rod slices.

## Equal probability and repetition control

Every eligible National Dex species is inserted exactly once into a transient EWRAM shuffle bag.

- Fisher-Yates randomizes the bag.
- Every species appears once before that pool repeats.
- The first species of a new bag is swapped when necessary to prevent an immediate cycle-boundary repeat.
- Map, encounter method, rod or inferred habitat changes rebuild the bag.
- Equal probability applies inside each route-local pool; global Hoenn frequency can differ because habitat-appropriate species occur in different numbers of local pools.
- Ambient cries use independent uniform sampling and do not consume encounter-bag entries.

There are no Phase 9 common/uncommon/rare species weights. Static and Magnet Pull species weighting are not applied to the Phase 9 picker.

## Dynamic levels

Only living, non-Egg members of the active party are counted.

`anchor = rounded party level average`

`wild level = uniform random(anchor - 15 ... anchor + 15)`

The result is clamped to `Lv.2 ... Lv.100`.

Species selection and level selection are independent, so evolved species are not assigned a hidden rarity or level penalty.

## Special-system boundaries

Normal Hoenn land, Surf, Rock Smash, Sweet Scent and fishing encounters use Phase 9.

Battle Pike and Battle Pyramid keep their own vanilla encounter generators. Roamers and mass outbreaks remain special encounter systems rather than members of the ordinary equal-probability pool.

Feebas's old six-tile fishing override is no longer used by `FishingWildEncounter`; Feebas participates through its waters-edge habitat like any other eligible species.

## Verification

`tools/vanillaplus_phase09_verify.py` checks:

- 386/386 habitat entries
- no canonical rarity bucket in runtime metadata
- equal one-copy shuffle-bag construction
- anti-repeat boundary handling
- party-average +/-15 level rules
- normal encounter routing
- Frontier isolation
- Feebas equal-probability fishing path
- route-local pools never exceed their fixed EWRAM capacity
- all 386 species occur in at least one normal route/method local pool
- save-layout isolation
- synchronized VP017/T017/V+017 markers

`.github/workflows/phase09-branch.yml` runs all completed-phase verifiers and builds release/test ROMs.
