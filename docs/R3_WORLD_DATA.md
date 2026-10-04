# R3 — Full Hoenn World Data Completion Evidence

Date: 2026-10-04  
Branch: `remaster/r3-world-data`  
Acceptance implementation head: `991ec4dd7ab917e641a473e304c19caf62794b4a`  
GitHub Actions acceptance run: `37179654056`

## Status

**R3 COMPLETE — full-Hoenn world-data conversion and acceptance validation passed.**

The remaster now converts the pinned real Vanilla+ world source into deterministic, audited intermediate data for the complete Hoenn map catalog. R3 remains a data/conversion phase: authoritative free-roaming movement, collision and playable overworld runtime begin in R4.

## Authoritative Vanilla+ provenance

The accepted source baseline is pinned to:

`illetyus/pokezumrut-vanillaplus@70db90c9077aed1272e746fc2537d9f12b95a91c`

Vendored snapshot Git tree:

`5a551f1f9e40184278c57dfb8d25f68a0a1c99dc`

Original Vanilla+ source Git tree recorded by the vendor manifest:

`58b83886d5f99915741a0929bd2d67a95a78740a`

The full-world probe verifies the pinned vendored tree before conversion. The vendored source is not modified to make the converter pass; compatibility differences are handled by the remaster conversion/audit layer and regression tests.

## Accepted full-world counts

The passing acceptance run measured:

- map groups: **34**
- maps: **518 / 518**
- layouts: **441**
- map script source files: **468**
- total script source files catalogued: **554**
- script labels catalogued: **17,339**
- encounter groups: **3**
- maps with encounter data: **124**
- generated files in the deterministic package: **964**

## Determinism and fingerprints

Two independent world conversions were generated and audited by the R3 full-world probe.

Result:

- audit A errors: **0**
- audit B errors: **0**
- deterministic comparison: **true**
- package SHA-256:
  `7567dee04ed2f48a4c39636042d865233d38b080200b837aadb1de12539c3d01`
- raw output tree SHA-256:
  `7e8158df81e280b100b011bf984945ecaa936c539a39262dc2e5a8a11ca8bb0c`

The two conversion passes are therefore byte-identical at the package/tree acceptance boundary.

## R3 world-data coverage

R3 converts and validates the complete map catalog while preserving source identities needed by later runtime phases.

Coverage includes:

- map group and map numeric identities;
- all 441 layouts;
- raw Emerald block/metatile words;
- border data;
- primary and secondary metatile attributes;
- collision/elevation source bits without inventing R4 gameplay semantics;
- map metadata;
- numeric music identities;
- numeric region-map-section identities;
- numeric battle-scene identities;
- object events;
- warp events;
- coordinate events;
- background events;
- map connections;
- dive/emerge connections;
- dynamic warp exceptions;
- shared event ownership;
- shared script ownership;
- event -> script references;
- map/warp/connection target identities;
- complete script source catalog;
- species identities;
- wild encounter groups/tables;
- package provenance and per-file SHA-256 fingerprints.

## Task 8 real-source compatibility findings

Full-Hoenn acceptance intentionally exposed source-shape differences that smaller fixtures did not cover. They were resolved without bypassing validation.

### Numeric zero `connections` sentinel

Several real Vanilla+ map JSON files encode no map connections as numeric `0` rather than JSON `null`.

Confirmed examples include:

- `AlteringCave`
- `ArtisanCave_B1F`
- `ShoalCave_LowTideIceRoom`
- `ShoalCave_LowTideInnerRoom`
- `ShoalCave_HighTideInnerRoom`

Vanilla+ map generation treats this as an empty/null connection pointer. The converter now normalizes only the exact integer-zero sentinel to an empty connection list.

Other scalar values remain invalid. In particular, booleans, non-zero integers and arbitrary strings are not silently accepted.

Regression coverage:

`test_real_vanillaplus_zero_connections_variant`

### Dive / emerge map connections

The real source uses two connection directions beyond the four cardinal directions:

- `dive` -> `CONNECTION_DIVE = 5`
- `emerge` -> `CONNECTION_EMERGE = 6`

The converter and generated-content audit now preserve and validate all six Emerald connection identities:

1. down
2. up
3. left
4. right
5. dive
6. emerge

The full source contains seven surface-to-underwater dive connections and the corresponding seven underwater-to-surface emerge connections in the accepted map set.

Regression coverage includes:

- `test_real_vanillaplus_dive_and_emerge_connection_ids`
- audit coverage for direction IDs 5 and 6

## Acceptance pipeline

The passing GitHub Actions run `37179654056` completed all four R0 Core workflow jobs successfully.

### r3-full-world

**SUCCESS**

The job:

1. verifies the pinned Vanilla+ source snapshot;
2. converts the full world into world A;
3. audits world A;
4. converts the same source independently into world B;
5. audits world B;
6. verifies deterministic package output;
7. records counts, provenance and fingerprints;
8. emits the R3 evidence artifact.

Final probe status:

`PASS`

### world-converter

**SUCCESS**

This job includes the R3 converter regression suites, source/script catalog tests, encounter conversion tests, package fingerprint tests, script-conversion regressions, generated-content audit, platform-coupling inventory and Unreal source architecture validation.

### portable-core

**SUCCESS**

R0/R1/R2 portable-core regressions remained green after the R3 changes.

### scenario-schema

**SUCCESS**

Existing scenario-schema validation remained green.

## R3 exit-gate result

- [x] pinned real Vanilla+ source used;
- [x] 34 map groups accounted for;
- [x] 518 / 518 maps converted;
- [x] 441 layouts converted;
- [x] 468 map script source files accounted for;
- [x] shared script/event ownership validated;
- [x] event -> script references audited;
- [x] warp targets audited;
- [x] map connection targets audited;
- [x] dynamic warp exceptions handled explicitly;
- [x] species identities validated;
- [x] encounter tables converted;
- [x] numeric music identities preserved;
- [x] numeric region identities preserved;
- [x] numeric battle-scene identities preserved;
- [x] audit A = 0;
- [x] audit B = 0;
- [x] two independent conversions deterministic;
- [x] package SHA-256 stable;
- [x] raw output tree SHA-256 stable;
- [x] R1/R2 regressions green;
- [x] Unreal architecture validation green;
- [x] full-world CI acceptance green.

**R3 exit gate: PASSED.**

## Next phase

R4 starts the authoritative playable overworld runtime.

The first vertical acceptance slice is:

**Littleroot player control -> leave the house through the real warp -> move through Littleroot -> cross the real map connection into Route 101.**

R4 will add movement/collision/elevation/warp/connection runtime semantics on top of the R3 data package. Battle remains out of scope for that phase.
