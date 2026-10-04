# R3 — Full Hoenn World Data Pipeline Design

Date: 2026-10-03  
Branch: `remaster/r3-world-data`  
Base: `R2 COMPLETE` (`e46d6fbea40e401a1cbc7e7c36982b25171b47f8`)  
Authoritative source baseline: `illetyus/pokezumrut-vanillaplus@70db90c9077aed1272e746fc2537d9f12b95a91c`

## 1. Goal

R3 turns the complete Vanilla+ Hoenn world source into a deterministic, auditable, platform-neutral world package that later runtime phases can consume without reading GBA ROM addresses or reinterpreting pokeemerald build-time structures.

R3 is a data-pipeline phase, not a movement, battle or presentation phase.

The R3 package must cover the complete authoritative source catalog:

- 34 map groups;
- 518 map JSON definitions;
- 441 layouts;
- 468 map-local script files plus shared script ownership;
- complete map geometry and border words;
- metatile attributes / behavior / layer data;
- map metadata;
- connections and warps;
- object, coord and background events;
- shared event/script relationships;
- stable R2 script identities;
- map-linked wild encounter data;
- provenance and deterministic content fingerprints.

R3 does not require every converted script opcode to be executable by the R2 runtime yet. It does require every source script reference and script source unit to be represented losslessly and classified so later phases do not have to rediscover the source graph.

## 2. Existing foundation

R1 already provides portable map/event/save/transition semantics.

R2 provides:
- stable script IDs;
- semantic script IR;
- source-to-IR conversion;
- native script fixture generation;
- runtime scheduling and host/domain boundaries.

The existing `tools/convert_world.py` already converts:
- every discovered map JSON;
- map-group numeric identity;
- layout identity;
- block/border data;
- metatile IDs;
- collision/elevation bits;
- metatile attributes/behavior/layer;
- weather/map-type identity;
- object/warp/coord/bg events;
- shared event maps;
- connection/warp numeric targets.

The existing `tools/audit_generated_content.py` already validates substantial per-map invariants.

R3 extends this from a capable map converter into a complete source-wide content pipeline with provenance, script/wild-data linkage, deterministic manifests and full-catalog acceptance evidence.

## 3. Source baseline facts

The pinned Vanilla+ baseline contains:

- 34 map groups;
- 518 entries in `data/maps/map_groups.json`;
- 518 corresponding `data/maps/*/map.json` files;
- 441 entries in `data/layouts/layouts.json`;
- 468 `data/maps/*/scripts.inc` files.

Maps without their own `scripts.inc` use shared script ownership or intentionally have no map-local script body. Shared events and shared scripts must be represented explicitly rather than inferred later.

R3 records these counts in the source-provenance acceptance contract. Any future baseline update that changes them must update provenance deliberately.

## 4. Canonical output package

R3 output root remains compatible with the existing Unreal loader:

`Content/Generated/World/`

Canonical package structure:

- `manifest.json`
- `provenance.json`
- `maps/<MapName>.json`
- `scripts/manifest.json`
- `scripts/programs.json` or equivalent deterministic split representation
- `encounters.json`
- `audit.json`

Generated files are build artifacts. The repository does not need to commit the full generated commercial world payload. Tests may commit minimal synthetic or provenance fixtures; source-wide generated packages should be CI/probe artifacts.

## 5. World manifest

The manifest remains the primary runtime lookup catalog.

Required top-level fields:

- schema version;
- source baseline commit;
- converter version;
- map count;
- layout count;
- map-group count;
- script-source count;
- encounter-map count;
- deterministic package fingerprint.

Each map entry contains at minimum:

- symbolic map ID;
- canonical map name;
- group name;
- numeric group/map address;
- layout symbolic/numeric ID;
- relative map file;
- script owner/source identity;
- shared events owner when present;
- shared scripts owner when present;
- map-data fingerprint.

Numeric map addresses are unique and must match `map_groups.json` order exactly.

## 6. Per-map canonical IR

Per-map JSON preserves source identity and resolved numeric identities side by side.

### 6.1 Map metadata

Preserve:
- map ID/name;
- music;
- region map section;
- flash requirement;
- weather;
- map type;
- cycling/escaping/running permissions;
- show-map-name;
- battle scene;
- layout;
- shared events/scripts relationships.

R3 additionally resolves numeric identities needed later where authoritative constants exist:
- music ID;
- region-map-section ID;
- battle-scene ID;
- connection direction ID;
- existing weather/map-type/layout IDs.

Unknown numeric symbols are hard conversion failures unless explicitly classified as dynamic/runtime-owned.

### 6.2 Layout data

For every map-referenced layout preserve:
- width/height;
- primary/secondary tileset identity;
- raw active block words;
- source trailing words;
- Emerald border active/trailing words;
- decoded metatile IDs;
- collision;
- elevation;
- complete primary/secondary metatile attribute words;
- decoded behavior;
- decoded layer.

The package additionally contains a layout catalog so every one of the 441 source layouts is either:
- referenced by at least one map; or
- explicitly listed as unreferenced/unused source data.

R3 must not silently omit a source layout.

### 6.3 Script references

A source script field with `0x0`, numeric zero or equivalent null sentinel is represented as no script, not as a stable script ID named `"0x0"`.

Non-null references preserve:
- source symbolic label;
- stable R2 `script_id`;
- resolved script owner/source unit where known.

Every non-null script reference in:
- object events;
- coord events;
- background events;
- map lifecycle scripts

must resolve into the R3 script catalog.

### 6.4 Object events

Preserve:
- local ID matching Vanilla array position;
- coordinates/elevation;
- graphics identity;
- movement identity;
- movement ranges;
- trainer type;
- trainer/berry identity;
- script identity;
- visibility flag.

R3 preserves trainer references but does not migrate trainer battle/team mechanics.

### 6.5 Warp events

Preserve:
- source coordinates/elevation;
- destination map;
- destination warp ID;
- numeric destination group/map;
- dynamic warp classification.

`MAP_DYNAMIC` remains an explicit dynamic target and is never treated as a missing catalog map.

### 6.6 Connections

Preserve:
- direction symbolic and numeric identity;
- offset;
- destination map symbolic and numeric identity.

R3 validates target existence and numeric address. It does not require every connection to have a reverse connection because Vanilla contains intentional asymmetric/dynamic behavior.

### 6.7 Coord events

Preserve:
- trigger/weather kind;
- coordinates/elevation;
- trigger var/value numeric identity;
- weather numeric identity;
- stable script identity.

### 6.8 Background events

Preserve:
- sign/facing;
- hidden items;
- secret-base IDs;
- script identity;
- numeric item/flag identities.

Unknown event kinds are hard failures.

## 7. Shared event/script ownership

R3 models sharing explicitly.

### Shared events

For `shared_events_map`:
- source owner map must exist;
- converted event arrays must be byte/semantic-equivalent to the owner;
- consumer map records the owner identity;
- source provenance identifies the actual owner map JSON.

### Shared scripts

For `shared_scripts_map`:
- owner map must exist or be a valid shared script namespace;
- consumer map records the script owner;
- no separate local `scripts.inc` is required;
- map event script references resolve against the shared/global script index.

A full-catalog audit proves that all 518 maps have a valid script ownership state:
- own scripts;
- shared scripts; or
- explicitly scriptless.

## 8. Full script source catalog

R3 extends R2 conversion from the opening dependency closure to the full Hoenn source graph.

The script catalog inventories:
- all 468 map-local `scripts.inc` files;
- shared map script owners;
- `data/scripts/**/*.inc`;
- `data/event_scripts.s`;
- movement/text labels referenced by those scripts;
- special IDs.

Requirements:

1. every source label gets one stable identity;
2. duplicate labels are rejected unless source semantics intentionally alias/rebind and the converter has an explicit rule;
3. every branch/call target resolves;
4. every map event script reference resolves;
5. every movement/text/special reference is represented;
6. every command is classified.

Runtime-deferred commands remain in IR as explicitly classified operations or generic lossless source operations. They are not silently discarded and are not claimed as executable until their owning gameplay phase implements them.

R3 must therefore distinguish:
- **conversion support** — source is represented losslessly;
- **runtime support** — R2/later core can execute it now.

## 9. Wild encounter data

`src/data/wild_encounters.json` is part of R3 because encounters are map-linked world content.

R3 converts/preserves:
- encounter group definitions;
- slot probability tables;
- rod group partitions;
- map association;
- encounter rates;
- min/max levels;
- species symbolic identity;
- species numeric identity where available.

R3 validates:
- every encounter map exists in the world catalog;
- slot counts match group field definitions;
- level ranges are valid;
- encounter rates are valid;
- species symbols resolve.

R3 does not implement RNG, encounter triggering, repel logic or battle creation. Those remain R12.

## 10. Provenance and fingerprints

`provenance.json` records:

- Vanilla+ repository identity;
- exact source commit;
- world schema version;
- converter/tool version or Git commit;
- authoritative source counts;
- SHA-256 hashes for source catalog inputs used by conversion;
- package fingerprint.

Each generated map contains a stable map fingerprint derived from canonical JSON content.

The package fingerprint is derived from canonical ordered hashes, not filesystem timestamps.

Running conversion twice against identical source and tools must produce byte-identical JSON outputs and the same package fingerprint.

## 11. Audit model

R3 audits are split into four layers.

### A. Source catalog parity
- map-groups count;
- map JSON count;
- layout count;
- map script file count;
- duplicate/missing map names/IDs;
- map-group ordering.

### B. Per-map structural audit
Existing geometry/event checks plus new numeric metadata/script ownership checks.

### C. Cross-map graph audit
- connection targets;
- warp targets;
- shared event owners;
- shared script owners;
- unique numeric map addresses;
- layout references;
- script references;
- encounter-map references.

### D. Determinism/provenance audit
- source commit matches pinned baseline;
- generated fingerprints match recomputation;
- second conversion produces identical output tree.

Audit output is machine-readable `audit.json` plus human-readable CI errors.

## 12. Real-source acceptance strategy

The remaster repository cannot assume the complete private Vanilla+ source tree is locally present in every ordinary CI job.

Therefore R3 uses two evidence layers:

1. **ordinary remaster CI**
   - synthetic converter unit tests;
   - committed minimal source fixtures;
   - provenance/catalog tests;
   - content-audit tests;
   - Unreal loader architecture checks.

2. **real Vanilla+ source probe**
   - executed against `pokezumrut-vanillaplus@70db90c...`;
   - converts the complete 518-map source tree;
   - converts the complete map-linked script catalog;
   - converts wild encounter data;
   - runs the full R3 audit;
   - runs conversion twice and compares output fingerprints;
   - uploads manifest/provenance/audit summaries as evidence artifacts.

The source-wide probe can live on a dedicated non-production Vanilla+ probe branch if cross-private-repository checkout credentials are unavailable. Its converter files must be hash-pinned to the remaster R3 head being validated.

## 13. Unreal/runtime boundary

The existing Unreal `URemasterWorldCatalogSubsystem` remains compatible with `manifest.json` and per-map JSON files.

R3 may extend the Unreal data structs/parser for additional numeric metadata, but:
- Unreal does not reinterpret gameplay semantics;
- Unreal does not own map/event truth;
- map package data remains portable;
- R4 consumes the same canonical map/event data for authoritative overworld movement.

No production art conversion is part of R3.

## 14. Deferred systems

R3 deliberately does not implement:

- free-roaming collision/movement behavior — R4;
- encounter triggering/RNG — R12;
- trainer battle execution/team data — R13;
- final tileset rendering/art — later presentation phases;
- final audio behavior;
- final quest/navigation UI;
- full item/Pokémon gameplay domain.

R3 only ensures later systems receive correct stable source data.

## 15. R3 exit gate

R3 is complete only when:

- [ ] pinned baseline provenance is recorded;
- [ ] 34/34 map groups are accounted for;
- [ ] 518/518 maps convert successfully;
- [ ] 441/441 layouts are accounted for;
- [ ] 468 map script files are inventoried and script ownership is complete for all maps;
- [ ] every non-null event script reference resolves;
- [ ] shared events relationships audit cleanly;
- [ ] shared scripts relationships audit cleanly;
- [ ] every connection target resolves;
- [ ] every non-dynamic warp target resolves;
- [ ] dynamic warps are explicitly preserved;
- [ ] all map-referenced metatile/block/border data converts;
- [ ] all map-linked wild encounter records convert and resolve;
- [ ] full script source catalog converts without silent command loss;
- [ ] conversion support vs runtime support is reported separately;
- [ ] two full conversions are byte/fingerprint deterministic;
- [ ] package provenance/fingerprints validate;
- [ ] ordinary remaster CI is green;
- [ ] full real-source R3 probe is green;
- [ ] generated manifest remains loadable by the Unreal world catalog;
- [ ] R1/R2 regression suites remain green.

**R3 closure means the complete Hoenn world exists as trusted portable data. It does not mean the player can freely walk all of Hoenn yet; that is R4.**
