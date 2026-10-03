# R3 Full Hoenn World Data Pipeline — Implementation Plan

**Goal:** Convert and audit the complete pinned Vanilla+ Hoenn world into a deterministic portable package while preserving R1/R2 compatibility and separating data conversion from later gameplay/runtime phases.

**Spec:** `docs/superpowers/specs/2026-10-03-r3-world-data-design.md`

**Branch:** `remaster/r3-world-data`

## Global rules

- TDD for every executable behavior change.
- Vanilla+ baseline remains pinned to `70db90c9077aed1272e746fc2537d9f12b95a91c`.
- Full generated commercial world payload is a build/probe artifact, not required to be committed.
- Every source reference is either resolved or explicitly classified dynamic/deferred.
- Do not broaden R3 into R4 movement, R12 encounters, or R13 battles.
- Preserve current Unreal `Content/Generated/World/manifest.json` loading contract.
- Keep R1/R2 tests green after each task.

---

## Task 1 — Source catalog and provenance contract

**Files**
- Create: `tools/r3_source_catalog.py`
- Create: `tests/test_r3_source_catalog.py`
- Create: `tests/fixtures/r3/vanillaplus_catalog.json`
- Modify: `.github/workflows/r0-core.yml`

**Behavior**
- Parse map groups and layouts.
- Count/discover map JSON and map-local script files.
- Record pinned source commit and authoritative source counts.
- Validate 34 groups / 518 maps / 441 layouts / 468 map script files for the pinned fixture.
- Generate deterministic canonical catalog JSON.
- Fail on duplicate/missing map identity or group-order mismatch.

**TDD**
1. Add failing synthetic tests for map/group/layout/script counting and duplicate identity errors.
2. Implement catalog builder.
3. Add pinned minimal catalog fixture with expected baseline counts and names/hashes derived from source metadata.
4. Add CI test.
5. Commit: `feat: add R3 source catalog provenance`.

---

## Task 2 — World converter canonical metadata and null-script semantics

**Files**
- Modify: `tools/convert_world.py`
- Modify: `tests/test_convert_world.py`
- Modify: `tools/audit_generated_content.py`
- Modify: `tests/test_content_audit.py`

**Behavior**
- Resolve numeric IDs for:
  - music;
  - region-map-section;
  - battle scene;
  - connection direction.
- Normalize script sentinels `0x0` / numeric zero to null/no-script.
- Preserve non-null stable `script_id`.
- Emit explicit script/shared ownership fields.
- Keep symbolic source values for audit/debug.

**TDD**
- Pin null-script conversion.
- Pin numeric connection direction.
- Pin unresolved required numeric metadata failure.
- Pin script ownership fields.
- Commit: `feat: canonicalize R3 map metadata and script refs`.

---

## Task 3 — Layout catalog and full structural coverage

**Files**
- Modify: `tools/convert_world.py`
- Modify: `tools/audit_generated_content.py`
- Modify: `tests/test_convert_world.py`
- Modify: `tests/test_content_audit.py`

**Behavior**
- Emit `layouts.json` catalog.
- Account for all source layouts, including unused/unreferenced layouts.
- Record map usage list per layout.
- Preserve source paths and hashes.
- Audit every map layout reference and every source layout accounting state.
- Detect duplicate numeric layout IDs.

**TDD**
- Synthetic used + unused layout case.
- Duplicate layout numeric ID.
- Missing map layout.
- Commit: `feat: catalog complete Vanilla layout data`.

---

## Task 4 — Shared events/scripts and complete map script ownership

**Files**
- Modify: `tools/convert_world.py`
- Modify: `tools/convert_scripts.py`
- Create/modify: `tools/r3_script_catalog.py`
- Create: `tests/test_r3_script_catalog.py`
- Modify: `tools/audit_generated_content.py`

**Behavior**
- Inventory all map-local `scripts.inc`.
- Resolve `shared_scripts_map`.
- Classify every map as own/shared/scriptless.
- Build a global script-label/source-owner index.
- Resolve all non-null map event script IDs against that index.
- Preserve shared-events parity.
- Report conversion support separately from runtime support.

**TDD**
- own/shared/scriptless maps;
- unresolved shared owner;
- unresolved event script label;
- duplicate label conflict;
- explicit runtime-deferred operation classification.
- Commit: `feat: build full Hoenn script ownership catalog`.

---

## Task 5 — Wild encounter conversion

**Files**
- Create: `tools/convert_encounters.py`
- Create: `tests/test_convert_encounters.py`
- Modify: `tools/audit_generated_content.py`
- Modify: `tests/test_content_audit.py`

**Behavior**
- Convert `src/data/wild_encounters.json`.
- Preserve encounter groups/slot weights/rod partitions.
- Resolve map IDs.
- Resolve species IDs.
- Validate levels/rates/slot counts.
- Output deterministic `encounters.json`.

**TDD**
- valid land/water/fishing group;
- bad map;
- bad species;
- invalid level range;
- wrong slot count.
- Commit: `feat: convert map-linked wild encounter data`.

---

## Task 6 — Provenance, canonical hashes and deterministic package

**Files**
- Create: `tools/world_fingerprint.py`
- Modify: `tools/convert_world.py`
- Modify: `tools/audit_generated_content.py`
- Create: `tests/test_world_fingerprint.py`

**Behavior**
- Canonical JSON serialization.
- Per-map SHA-256.
- layout/script/encounter hashes.
- `provenance.json`.
- package fingerprint.
- audit recomputation.
- two identical conversions produce byte-identical files/fingerprint.

**TDD**
- key-order independence;
- content-change fingerprint change;
- timestamp ignored;
- tampered map/provenance failure.
- Commit: `feat: fingerprint deterministic world packages`.

---

## Task 7 — Unreal world package compatibility and package-level audit

**Files**
- Modify: Unreal world data/catalog parser only where new manifest fields require it.
- Modify: `tools/validate_unreal_source.py`
- Add focused C++/fixture tests as needed.
- Modify: `CMakeLists.txt`.

**Behavior**
- Existing loader still resolves numeric map addresses.
- New manifest/provenance fields are tolerated/read.
- No gameplay semantics move into Unreal.
- Synthetic complete package loads successfully.
- R1/R2 portable core and Unreal embed tests remain green.

**Commit**
`test: validate R3 package against Unreal catalog`.

---

## Task 8 — Real Vanilla+ full-source probe and R3 closure

**Files**
- Create: `docs/R3_WORLD_DATA.md`
- Add probe tooling/workflow on a dedicated Vanilla+ probe branch if cross-private checkout remains unavailable.
- Modify CI only as required to publish evidence.

**Probe requirements**
- Source exactly `70db90c...`.
- Run complete world conversion.
- 34/34 groups.
- 518/518 maps.
- 441/441 layouts accounted.
- 468 map-local script files inventoried.
- shared script/event ownership resolves.
- all non-null event script references resolve.
- all connection/warp targets resolve except explicit dynamic warps.
- all map-linked encounter records resolve.
- full audit = zero errors.
- run conversion twice; package fingerprints identical.
- upload:
  - manifest summary;
  - provenance;
  - audit report;
  - fingerprints;
  - counts.

**Closure**
- Record real-source evidence in `docs/R3_WORLD_DATA.md`.
- Add `R3 COMPLETE` commit.
- Merge/fast-forward to `main` only after final green evidence.

Commit before closing:
`docs: record R3 full-world conversion evidence`.

---

## Final R3 checklist

- [ ] source catalog/provenance contract green;
- [ ] canonical numeric metadata complete;
- [ ] null script sentinels normalized;
- [ ] 441 layouts accounted;
- [ ] shared event/script ownership complete;
- [ ] all map script references resolve;
- [ ] wild encounter conversion complete;
- [ ] deterministic package hashes validated;
- [ ] Unreal manifest loader compatibility validated;
- [ ] full real-source probe passes 518-map conversion;
- [ ] two-run determinism proven;
- [ ] R1/R2 regressions green;
- [ ] evidence doc committed;
- [ ] `R3 COMPLETE` committed.
