# Vanilla+ Phase 4 HM Field Tools Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make HM01–HM08 usable as badge-gated field tools when the HM item is in the bag, without requiring any party Pokémon to know the HM move and without changing battle movesets or serialized save layout.

**Architecture:** Add a small `vanillaplus_hm` policy module that owns the explicit HM item/move/badge mapping and safe field-effect actor selection. Route HM bag use to field setup instead of the TM/HM teaching screen, remove learned-move authorization from overworld scripts and automatic Surf/Dive/Waterfall paths, and keep existing Emerald field effects, map restrictions, story flags, and special puzzle behavior.

**Tech Stack:** pokeemerald C, Emerald event scripts, Python source-contract verifiers, GitHub Actions, agbcc/ARM toolchain.

**Spec:** `docs/superpowers/specs/2026-10-01-phase-4-hm-field-tools-design.md`

## Global Constraints

- `GAME_CODE` remains `BPEE`.
- `SaveBlock1`, `SaveBlock2`, `PokemonStorage`, Pokémon serialized data and item/species/move IDs must not move or change size.
- Existing 128 KiB saves must remain load/save compatible.
- HM field access requires both the HM item and the vanilla badge.
- HM learnset compatibility and battle mechanics are not changed.
- HM field use must not teach, delete, or replace a move.
- Existing learned HM moves remain untouched and removable through the Phase 2 behavior.
- Phase 3 field animation shortening and Flash full-visibility behavior remain intact.
- Link/Union Room restrictions must not gain a new bypass.
- Phase 4 build identity is `ZUMRUT VP005`, `ZUMRUT T005`, `OYUNCU V+005`.

## Review Focus

- A save with the correct badge but without the HM item must never gain field access.
- A save with the HM item but without the required badge must never gain field access.
- A party with no Pokémon knowing the HM must still work, while an invalid/empty actor index must never reach a field effect.
- Surf/Dive/Waterfall automatic interaction paths must use the same HM-item gate as bag use and must not bypass map/direction requirements.
- Registeel/Regirock special puzzle paths and existing Strength/Rock Smash side effects must continue to work after party-menu decoupling.

---

### Task 1: Add the HM policy and field-actor module

**Files:**
- Create: `include/vanillaplus_hm.h`
- Create: `src/vanillaplus_hm.c`
- Create/extend test: `tools/vanillaplus_phase04_verify.py`

**Interfaces:**
- Produces: `bool8 IsVanillaPlusHmItem(u16 itemId)`
- Produces: `u16 GetVanillaPlusHmMove(u16 itemId)`; returns `MOVE_NONE` for non-HMs.
- Produces: `u16 GetVanillaPlusHmBadgeFlag(u16 itemId)`; returns `0` for non-HMs.
- Produces: `bool8 HasVanillaPlusHmAccess(u16 itemId)`; true only when the item is in the bag and its badge flag is set.
- Produces: `u8 GetVanillaPlusHmFieldActor(void)`; first non-Egg party Pokémon or `PARTY_SIZE` when none exists.

- [ ] **Step 1: Write the failing source-contract checks**

Add Phase 4 verifier assertions for the two new files, the four API names, all eight explicit mappings, and actor rejection of Eggs/no-valid-party cases. The exact HM mapping is:

```text
HM01/CUT        -> FLAG_BADGE01_GET
HM05/FLASH      -> FLAG_BADGE02_GET
HM06/ROCK_SMASH -> FLAG_BADGE03_GET
HM04/STRENGTH   -> FLAG_BADGE04_GET
HM03/SURF       -> FLAG_BADGE05_GET
HM02/FLY        -> FLAG_BADGE06_GET
HM08/DIVE       -> FLAG_BADGE07_GET
HM07/WATERFALL  -> FLAG_BADGE08_GET
```

- [ ] **Step 2: Run the verifier and confirm failure**

Run: `python3 tools/vanillaplus_phase04_verify.py`
Expected: FAIL because the policy module does not exist yet.

- [ ] **Step 3: Implement the policy module**

Use table/switch-based explicit mapping; do not derive badge flags from HM numeric order. `HasVanillaPlusHmAccess()` uses `CheckBagHasItem(itemId, 1)` plus `FlagGet()`. `GetVanillaPlusHmFieldActor()` ignores fainted state, rejects Eggs, and never returns an invalid index as a successful actor.

- [ ] **Step 4: Run the focused verifier**

Run: `python3 tools/vanillaplus_phase04_verify.py`
Expected: the Task 1 policy checks PASS; later-phase checks may still fail until subsequent tasks.

- [ ] **Step 5: Commit**

```bash
git add include/vanillaplus_hm.h src/vanillaplus_hm.c tools/vanillaplus_phase04_verify.py
git commit -m "feat: add phase 4 HM field policy"
```

### Task 2: Route HM items to field use instead of move teaching

**Files:**
- Modify: `src/data/items.h` HM01–HM08 entries
- Modify: `include/item_use.h`
- Modify: `src/item_use.c`
- Modify: `tools/vanillaplus_phase04_verify.py`

**Interfaces:**
- Consumes: Task 1 `IsVanillaPlusHmItem`, `HasVanillaPlusHmAccess`, `GetVanillaPlusHmFieldActor`.
- Produces: `void ItemUseOutOfBattle_HMField(u8 taskId)`.
- Produces: one HM dispatcher/setup path that preserves existing map checks and existing field effects.

- [ ] **Step 1: Add failing verifier assertions**

Assert HM01–HM08 no longer use `ITEM_USE_PARTY_MENU` + `ItemUseOutOfBattle_TMHM`, TM01–TM50 still do, and `ItemUseOutOfBattle_HMField` exists. Pin a Link/Union Room denial check in the new HM field-use path so bag use cannot bypass the restriction formerly enforced by party-menu field moves.

- [ ] **Step 2: Run the verifier and confirm failure**

Run: `python3 tools/vanillaplus_phase04_verify.py`
Expected: FAIL on HM item routing.

- [ ] **Step 3: Split TM and HM item behavior**

Keep `ItemUseOutOfBattle_TMHM()` for TM01–TM50 unchanged. Change HM01–HM08 to field-use routing and implement `ItemUseOutOfBattle_HMField(u8 taskId)`. It must reject missing badge, invalid context, Link/Union restrictions, or missing visual actor without consuming the HM. It must not open the party teach screen.

- [ ] **Step 4: Reuse the existing field setup paths**

For Cut, Flash, Rock Smash, Strength, Surf, Fly, Dive and Waterfall, reuse the existing setup/effect behavior rather than introducing parallel mechanics. Where setup currently lives as a `static` function in `src/party_menu.c`, move only the HM-specific setup/callback needed by the dispatcher into the HM module or expose a narrowly scoped helper; do not duplicate the body.

- [ ] **Step 5: Run verifier and compile**

Run: `python3 tools/vanillaplus_phase04_verify.py`
Expected: Task 2 checks PASS.

Run: `make -j$(nproc) all`
Expected: release build succeeds.

- [ ] **Step 6: Commit**

```bash
git add src/data/items.h include/item_use.h src/item_use.c include/vanillaplus_hm.h src/vanillaplus_hm.c src/party_menu.c tools/vanillaplus_phase04_verify.py
git commit -m "feat: use HMs as field tools from bag"
```

### Task 3: Remove HM field actions from Pokémon movesets and cursor-dependent actors

**Files:**
- Modify: `src/party_menu.c`
- Modify if still referenced: `src/data/party_menu.h`
- Modify: `src/fldeff_cut.c`
- Modify: `src/fldeff_flash.c`
- Modify: `src/fldeff_rocksmash.c`
- Modify: `src/fldeff_strength.c`
- Modify: `src/field_effect.c` only where Fly actor selection is cursor-dependent
- Modify: `tools/vanillaplus_phase04_verify.py`

**Interfaces:**
- Consumes: Task 1 `GetVanillaPlusHmFieldActor()`.
- Produces: HM field effects that no longer require `GetCursorSelectionMonId()` as their actor source.

- [ ] **Step 1: Add failing verifier assertions**

Assert `SetPartyMonFieldSelectionActions()` does not append the eight HM field actions from learned moves. Assert HM effect paths do not use `GetCursorSelectionMonId()` as the required actor source. Leave Dig, Teleport, Secret Power, Milk Drink, Soft-Boiled and Sweet Scent party behavior intact.

- [ ] **Step 2: Run verifier and confirm failure**

Run: `python3 tools/vanillaplus_phase04_verify.py`
Expected: FAIL on party-menu HM actions and cursor actor use.

- [ ] **Step 3: Remove only HM actions from the Pokémon field action list**

Keep non-HM field moves unchanged. Remove the old assumption that `FIELD_MOVE_CUT..FIELD_MOVE_WATERFALL` plus enum ordering is the authoritative badge system.

- [ ] **Step 4: Replace HM cursor actor reads**

Cut/Flash/Rock Smash/Strength/Surf/Dive/Waterfall/Fly field-effect argument setup must use Task 1’s safe actor. Preserve any effect code that depends on the actor species/ability for visuals, but do not use learnability or learned moves as authorization.

- [ ] **Step 5: Run verifier and both build configurations**

Run: `python3 tools/vanillaplus_phase04_verify.py`
Expected: Task 3 checks PASS.

Run: `make mostlyclean && make -j$(nproc) all`
Expected: PASS.

Run: `make mostlyclean && make -j$(nproc) VANILLAPLUS_TEST=1 all`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add src/party_menu.c src/data/party_menu.h src/fldeff_cut.c src/fldeff_flash.c src/fldeff_rocksmash.c src/fldeff_strength.c src/field_effect.c tools/vanillaplus_phase04_verify.py
git commit -m "refactor: decouple HM field effects from party moves"
```

### Task 4: Convert direct overworld Cut, Rock Smash, Strength and Surf checks

**Files:**
- Modify: `data/scripts/field_move_scripts.inc`
- Modify: `data/scripts/surf.inc`
- Modify: `src/field_control_avatar.c`
- Modify: `tools/vanillaplus_phase04_verify.py`

**Interfaces:**
- Consumes: Task 1 HM ownership/badge contract and safe actor.
- Produces: A-button/direct-map interaction paths that authorize from HM item + badge, not `checkpartymove`/`PartyHasMonWithSurf`.

- [ ] **Step 1: Add failing verifier assertions**

Require `checkitem ITEM_HM01_CUT`, `ITEM_HM06_ROCK_SMASH`, and `ITEM_HM04_STRENGTH` in their direct interaction scripts; prohibit their old `checkpartymove` gates. Require Surf’s `GetInteractedWaterScript()` gate to check HM03 ownership and Badge 5 instead of `PartyHasMonWithSurf()`. Require `data/scripts/surf.inc` to stop using `checkpartymove MOVE_SURF`.

- [ ] **Step 2: Run verifier and confirm failure**

Run: `python3 tools/vanillaplus_phase04_verify.py`
Expected: FAIL on learned-move gates.

- [ ] **Step 3: Update Cut/Rock Smash/Strength scripts**

Preserve existing yes/no prompts, object removal, `FLAG_SYS_USE_STRENGTH`, Rusturf Tunnel updates and Rock Smash encounter calls. Replace only authorization and nickname/move-message assumptions that require an HM-knowing Pokémon.

- [ ] **Step 4: Update Surf interaction**

`GetInteractedWaterScript()` requires Badge 5 + HM03 + `IsPlayerFacingSurfableFishableWater()`. `EventScript_UseSurf` uses the safe actor/effect argument rather than `checkpartymove`. Preserve Surf blob, follower and encounter behavior.

- [ ] **Step 5: Run verifier and build**

Run: `python3 tools/vanillaplus_phase04_verify.py && make -j$(nproc) all`
Expected: PASS for Task 4 contracts and compilation.

- [ ] **Step 6: Commit**

```bash
git add data/scripts/field_move_scripts.inc data/scripts/surf.inc src/field_control_avatar.c tools/vanillaplus_phase04_verify.py
git commit -m "feat: gate field obstacles by HM items"
```

### Task 5: Convert Dive/Waterfall and preserve Fly/Flash/Rock Smash puzzle behavior

**Files:**
- Modify: `data/scripts/field_move_scripts.inc`
- Modify: `src/field_control_avatar.c`
- Modify: `src/fldeff_flash.c`
- Modify: `src/fldeff_rocksmash.c`
- Modify: `src/party_menu.c` or HM module for moved Fly setup
- Modify: `src/field_effect.c` if Fly still obtains actor from party cursor
- Modify: `tools/vanillaplus_phase04_verify.py`

**Interfaces:**
- Consumes: Task 1 HM policy/actor; Task 2 HM bag dispatcher.
- Produces: HM08-gated Dive down/surface, HM07-gated Waterfall, HM02-gated Fly map flow, HM05-gated Flash including Registeel path, and preserved Emerald Regirock Rock Smash puzzle path.

- [ ] **Step 1: Add failing verifier assertions**

Assert `TrySetupDiveDownScript()` and `TrySetupDiveEmergeScript()` require HM08 + Badge 7 and retain `TrySetDiveWarp()` semantics. Assert Waterfall selection requires HM07 + Badge 8 + north-surf/waterfall checks and its script has no `checkpartymove MOVE_WATERFALL`. Assert Fly retains `Overworld_MapTypeAllowsTeleportAndFly()` and `CB2_OpenFlyMap`. Assert Flash retains `ShouldDoBrailleRegisteelEffect()`, `SetUpPuzzleEffectRegisteel`, and Phase 3 `setflashlevel 0`. Assert Rock Smash setup retains `ShouldDoBrailleRegirockEffect()` and `SetUpPuzzleEffectRegirock()` while no learned Rock Smash move is required.

- [ ] **Step 2: Run verifier and confirm failure**

Run: `python3 tools/vanillaplus_phase04_verify.py`
Expected: FAIL until special paths are converted.

- [ ] **Step 3: Convert Dive and Waterfall authorization**

Add HM ownership to the existing early C gates and remove script `checkpartymove` calls. Preserve `TrySetDiveWarp()` return meanings, underwater map-type check, `IsPlayerSurfingNorth()`, waterfall metatile check and existing field effects.

- [ ] **Step 4: Convert Fly bag flow**

HM02 + Badge 6 + `Overworld_MapTypeAllowsTeleportAndFly()` opens the existing Fly map via `CB2_OpenFlyMap`; selecting a destination must still run the existing Fly field effect/warp path with the safe actor.

- [ ] **Step 5: Convert Flash and Rock Smash without losing puzzle behavior**

HM05 + Badge 2 authorizes both normal cave Flash and the Emerald Registeel Flash puzzle route. Preserve `FLAG_SYS_USE_FLASH`, `ShouldDoBrailleRegisteelEffect()`, `SetUpPuzzleEffectRegisteel`, and `data/scripts/flash.inc` full visibility. HM06 + Badge 3 must still reach the existing Regirock Rock Smash puzzle path through `ShouldDoBrailleRegirockEffect()` / `SetUpPuzzleEffectRegirock()`.

- [ ] **Step 6: Run verifier and both builds**

Run: `python3 tools/vanillaplus_phase04_verify.py`
Expected: all behavioral source checks PASS except final CI/version integration if not yet done.

Run: `make mostlyclean && make -j$(nproc) all && make mostlyclean && make -j$(nproc) VANILLAPLUS_TEST=1 all`
Expected: both builds succeed.

- [ ] **Step 7: Commit**

```bash
git add data/scripts/field_move_scripts.inc src/field_control_avatar.c src/fldeff_flash.c src/fldeff_rocksmash.c src/party_menu.c src/field_effect.c tools/vanillaplus_phase04_verify.py
git commit -m "feat: complete phase 4 HM field paths"
```

### Task 6: Wire Phase 4 regression ownership, CI and final validation

**Files:**
- Modify: `tools/vanillaplus_phase04_verify.py`
- Modify: `tools/phase_version_contract_test.py`
- Modify: `.github/workflows/build.yml`
- Modify: `Makefile`
- Modify: `src/strings.c`

**Interfaces:**
- Consumes: completed Tasks 1–5.
- Produces: Phase 4 exact version ownership and CI enforcement.

- [ ] **Step 1: Add the final failing version/CI checks**

Phase 4 verifier owns exact marker `005`. Extend `phase_version_contract_test.py` so Phase 1–3 verifiers are not exact owners of newer markers and Phase 4 is. Require `.github/workflows/build.yml` to run Phase 4 verification after Phase 3.

- [ ] **Step 2: Run contract tests and confirm failure before marker/CI edits**

Run: `python3 tools/phase_version_contract_test.py && python3 tools/vanillaplus_phase04_verify.py`
Expected: FAIL on Phase 4 ownership/CI integration.

- [ ] **Step 3: Bump all three build markers and CI**

Set `ZUMRUT VP005`, `ZUMRUT T005`, and `OYUNCU V+005`; add `python3 tools/vanillaplus_phase04_verify.py` to the build workflow. Do not make older completed-phase verifiers claim exact marker `005`.

- [ ] **Step 4: Run the complete automated regression suite**

Run:

```bash
python3 tools/phase_version_contract_test.py
python3 tools/vanillaplus_phase01_verify.py
python3 tools/vanillaplus_phase02_verify.py
python3 tools/vanillaplus_phase03_verify.py
python3 tools/vanillaplus_phase04_verify.py
```

Expected: every verifier prints `PASSED` and exits 0.

- [ ] **Step 5: Build clean release and test ROMs**

Run:

```bash
make mostlyclean
make -j$(nproc) all
cp pokeemerald.gba pokemon-emerald_v1.gba
make mostlyclean
make -j$(nproc) VANILLAPLUS_TEST=1 all
cp pokeemerald.gba pokemon-emerald_v1-test.gba
```

Expected: both ROMs compile with no errors.

- [ ] **Step 6: Perform smoke-test matrix with a save where no party Pokémon knows the tested HM**

Verify: Cut HM01+Badge1; Flash HM05+Badge2 including Registeel path; Rock Smash HM06+Badge3 including Rusturf/encounter and Regirock puzzle path; Strength HM04+Badge4; Surf HM03+Badge5; Fly HM02+Badge6; Dive down and surface HM08+Badge7; Waterfall HM07+Badge8. For at least Cut, Surf and Dive, separately verify item-missing and badge-missing denial. Verify at least one HM is denied in Link/Union Room context. Then perform old-save `Continue -> Save -> reboot -> Continue`.

- [ ] **Step 7: Confirm Phase 3 regressions remain good**

Check running, Repel reuse, easier fishing, low-HP silence, poison floor at 1 HP, full-visibility Flash, bike toggle and shortened HM animation.

- [ ] **Step 8: Record hashes and commit**

Run:

```bash
sha1sum pokemon-emerald_v1.gba pokemon-emerald_v1-test.gba
sha256sum pokemon-emerald_v1.gba pokemon-emerald_v1-test.gba
```

Then:

```bash
git add tools/vanillaplus_phase04_verify.py tools/phase_version_contract_test.py .github/workflows/build.yml Makefile src/strings.c
git commit -m "ci: verify Vanilla+ phase 4 HM field tools"
```

## Completion Criteria

Phase 4 is complete only when all source verifiers pass, both clean builds succeed, all eight HM smoke tests work with no party Pokémon knowing the HM, missing-item/missing-badge negative tests deny access, Registeel/Regirock special paths pass, Link/Union Room does not gain an HM bypass, old-save save/reboot compatibility passes, and no Phase 3 behavior regresses.
