# Phase 5 Item Management Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete Bag/PC sorting, held-item move/swap, and up-to-four quick registered items while preserving the vanilla Emerald save layout.

**Architecture:** A new `vanillaplus_items` module owns sort comparison, Turkish collation, reserved-byte metadata and quick-item policy. Existing Bag, PC and Party UIs call that shared policy. SaveBlock structures are not extended; metadata lives behind accessors over the existing `unused_3598` reserve.

**Tech Stack:** C / pret pokeemerald decompilation, Python static verifier, GitHub Actions, agbcc/GBA build.

**Spec:** `docs/superpowers/specs/2026-10-01-phase-5-item-management-design.md`

## Global Constraints

- No SaveBlock1/SaveBlock2 size or offset changes.
- No Battle Pyramid bag behavior changes.
- Preserve existing SELECT manual Bag move.
- Preserve canonical TM/HM and Berry ordering.
- Old saves initialize Phase 5 metadata lazily and safely.
- Both release and `VANILLAPLUS_TEST=1` ROMs must build.

## Review Focus

- Garbage/non-Phase-5 bytes in the reserved save region must never be treated as valid metadata.
- Sorting encrypted Bag quantities must not compare or rewrite encrypted values as plaintext.
- Sorting must not lose duplicate stacks or quantities.
- Quick-item registration must prune items no longer in the Bag and must preserve existing single registered-item use.
- Held-item MOVE must not route Mail through generic held-item swapping.

---

### Task 1: Phase 5 contract and save-layout guard

**Files:**
- Create: `tools/vanillaplus_phase05_verify.py`
- Modify: `.github/workflows/build.yml`
- Modify: `tools/phase_version_contract_test.py`

**Interfaces:**
- Produces: static Phase 5 contract consumed by every later task.

- [ ] Write verifier checks before production code.
- [ ] Run verifier and observe failure because Phase 5 APIs do not exist.
- [ ] Keep checks for original `unused_3598[0x180]` declaration and no new SaveBlock fields.
- [ ] Add Phase 5 verifier to permanent build workflow.

### Task 2: Common item QoL core

**Files:**
- Create: `include/vanillaplus_items.h`
- Create: `src/vanillaplus_items.c`

**Interfaces:**
- Produces: `VanillaPlusSortBagPocket`, `VanillaPlusSortPCItems`, sort-mode metadata accessors, auto-sort accessors, quick-item accessors/policy.

- [ ] Implement reserved-region metadata magic/version/default initialization.
- [ ] Implement Turkish-aware item-name comparator.
- [ ] Implement Name/Type/Quantity/Value comparators with deterministic item-ID tie break.
- [ ] Implement Bag encrypted-quantity and PC plaintext-quantity sorting paths.
- [ ] Implement four-entry quick-item register/unregister/prune helpers.
- [ ] Run Phase 5 verifier.

### Task 3: Bag sorting and auto-sort UI

**Files:**
- Modify: `src/item_menu.c`
- Modify: `include/item_menu.h`
- Modify: `src/strings.c`
- Modify: `include/strings.h`

**Interfaces:**
- Consumes Task 2 sort and metadata APIs.
- Produces START sort menu and safe refresh auto-sort integration.

- [ ] Add START sort menu for normal field/battle/shop Bag contexts without breaking SELECT manual move.
- [ ] Add Name/Type/Quantity/Value/Auto/Cancel actions and Turkish labels.
- [ ] Keep cursor on the same item ID after sorting when possible.
- [ ] Apply auto-sort only at list refresh/open boundaries.
- [ ] Show quick-item registration state in Key Items pocket.
- [ ] Run Phase 5 verifier.

### Task 4: PC item sorting

**Files:**
- Modify: `src/player_pc.c`

**Interfaces:**
- Consumes Task 2 PC sorter.

- [ ] Add START sorting while browsing stored items.
- [ ] Rebuild list safely and preserve the selected item where possible.
- [ ] Keep SELECT manual PC item movement intact.
- [ ] Run Phase 5 verifier.

### Task 5: Party held-item MOVE/SWAP

**Files:**
- Modify: `src/party_menu.c`
- Modify: `src/data/party_menu.h`
- Modify: `src/strings.c`
- Modify: `include/strings.h`

**Interfaces:**
- Produces Party Item submenu GIVE / TAKE / MOVE / CANCEL.

- [ ] Add MOVE action only for ordinary held items.
- [ ] Ask for destination Pokémon using party selection flow.
- [ ] Move to empty destination or swap with occupied destination.
- [ ] Refresh both held-item sprites and descriptions.
- [ ] Reject eggs/invalid slots as existing party-selection policy requires and keep Mail on its existing path.
- [ ] Run Phase 5 verifier.

### Task 6: Multi quick items

**Files:**
- Modify: `src/item_menu.c`
- Modify: `src/field_control_avatar.c` only if required by the selected chooser flow.
- Modify: `src/strings.c`
- Modify: `include/strings.h`

**Interfaces:**
- Consumes Task 2 quick-item policy.

- [ ] Register/deregister up to four legal field-use key items.
- [ ] Keep `gSaveBlock1Ptr->registeredItem` synchronized with quick slot 0 for vanilla compatibility.
- [ ] Preserve direct SELECT use when only one valid quick item exists.
- [ ] Open chooser when multiple quick items exist and invoke existing field-use callback for the selected item.
- [ ] Prune missing items safely.
- [ ] Run Phase 5 verifier.

### Task 7: Version ownership, full regression and release build

**Files:**
- Modify: `Makefile`
- Modify: `src/strings.c`
- Modify: permanent CI files as required.

**Interfaces:**
- Produces synchronized `VP007/T007/V+007` build identity.

- [ ] Transfer exact version ownership to Phase 5 and bump release/test/Continue markers to 007.
- [ ] Run phase-version contract and Phase 0–5/8 verifiers.
- [ ] Build release ROM.
- [ ] Clean and build `VANILLAPLUS_TEST=1` ROM.
- [ ] Record hashes in Actions output.
- [ ] Merge only after every verification is green.
