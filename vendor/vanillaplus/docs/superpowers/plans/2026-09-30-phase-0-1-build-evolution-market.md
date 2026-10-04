# Vanilla+ Phase 0-1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stabilize the Vanilla+ build/test pipeline and finish the evolution + normal Poké Mart changes without changing the Emerald save layout.

**Architecture:** Keep all gameplay changes source-level and ID-compatible with vanilla Emerald. Add a source verifier that statically checks Phase 0-1 invariants, then compile both the normal development build and a test build from the same source using a build-time flag. Do not add permanent debug gameplay hooks in this phase; later phases may compile such hooks only under the test flag.

**Tech Stack:** C/ASM pokeemerald decomp, GNU Make, agbcc/arm-none-eabi, Python 3 source verifier, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-30-pokemon-emerald-vanillaplus-design.md`

## Global Constraints

- Final user-facing ROM filename is `pokemon-emerald_v1.gba`.
- Game code remains `BPEE`.
- Do not change `SaveBlock1`, `SaveBlock2`, `PokemonStorage`, item/species/move ID layouts, Mystery Gift/event-ticket storage, or legendary/event progression data.
- Rare Candy is not added to normal Poké Marts.
- All six evolution stones and PP Max are available in every normal Poké Mart inventory from the beginning.
- Only Fire/Water/Thunder/Leaf/Moon/Sun Stones are reusable after successful evolution.
- King’s Rock, Metal Coat, Dragon Scale, Up-Grade, DeepSeaTooth and DeepSeaScale must be directly usable for their configured item evolutions and remain consumable.
- Development build identifier for this phase is `VP002`; test builds must be distinguishable and debug/test-only features must never leak into the release build.

## Review Focus

- Multi-stage marts such as Oldale, Petalburg and Rustboro must contain the Phase 1 stock in every stock branch.
- NPC dialogue mentioning Rare Candy must not be mistaken for Rare Candy being sold.
- Reusable-stone logic must not make trade-evolution items or PP Max non-consumable.
- Direct-use conversion for trade-evolution items must not alter their item IDs, prices, hold effects, or save representation.
- Build/test flags must not alter the `BPEE` game code or save binary layout.

---

### Task 1: Add Phase 0-1 source verifier and observe RED

**Files:**
- Create: `tools/vanillaplus_phase01_verify.py`
- Create: `.github/workflows/phase01-verify.yml`

**Interfaces:**
- Consumes: repository source tree.
- Produces: exit code 0 only when all Phase 0-1 source invariants are present.

- [ ] **Step 1: Write the verifier**

The verifier must inspect all 11 normal Mart script files and every `*_Pokemart*` block, require `ITEM_ULTRA_BALL`, `ITEM_MAX_REVIVE`, the six evolution stones and `ITEM_PP_MAX`, reject `ITEM_RARE_CANDY` inside those inventory blocks, verify direct-use trade-evolution item definitions, verify the six-stone non-consumption guard, and verify VP002/test-build + artifact naming configuration.

- [ ] **Step 2: Run verifier in Actions before implementation**

Run: `python3 tools/vanillaplus_phase01_verify.py`
Expected: FAIL because Phase 1 stock and reusable/direct-use evolution behavior are not complete yet.

- [ ] **Step 3: Commit RED test**

Commit message: `test: add phase 0-1 Vanilla+ verifier`

### Task 2: Stabilize build identities and artifact names

**Files:**
- Modify: `Makefile`
- Modify: `src/strings.c`
- Modify: `.github/workflows/build.yml`

**Interfaces:**
- Produces: normal build identifier `ZUMRUT VP002`, test build identifier `ZUMRUT T002`, compile-time macro `VANILLAPLUS_TEST`, artifact `pokemon-emerald_v1.gba` and test artifact `pokemon-emerald_v1-test.gba`.

- [ ] **Step 1: Add `VANILLAPLUS_TEST ?= 0` and pass it through `CPPFLAGS`**
- [ ] **Step 2: Make header title conditional: normal `ZUMRUT VP002`, test `ZUMRUT T002`; keep `GAME_CODE := BPEE`**
- [ ] **Step 3: Change Continue screen build marker to `OYUNCU V+002`**
- [ ] **Step 4: Update Actions to run verifier, build release, hash it, clean/rebuild with `VANILLAPLUS_TEST=1`, and upload both exact filenames**
- [ ] **Step 5: Commit**

Commit message: `build: add VP002 release and test builds`

### Task 3: Complete normal Mart inventories and direct item evolutions

**Files:**
- Modify: `data/maps/{FallarborTown,FortreeCity,LavaridgeTown,MauvilleCity,MossdeepCity,OldaleTown,PetalburgCity,RustboroCity,SlateportCity,SootopolisCity,VerdanturfTown}_Mart/scripts.inc`
- Modify: `src/data/items.h`

**Interfaces:**
- Produces: every normal Poké Mart stock branch includes Ultra Ball, Max Revive, six evolution stones and PP Max; trade-evolution held items can invoke the party evolution-item UI.

- [ ] **Step 1: Add the seven Phase 1 items before `ITEM_NONE` in every normal Mart inventory block**
- [ ] **Step 2: Confirm no inventory block contains `ITEM_RARE_CANDY`**
- [ ] **Step 3: For King’s Rock, Metal Coat, Dragon Scale, Up-Grade, DeepSeaTooth and DeepSeaScale, set `.type = ITEM_USE_PARTY_MENU` and `.fieldUseFunc = ItemUseOutOfBattle_EvolutionStone` without changing other fields**
- [ ] **Step 4: Commit**

Commit message: `feat: complete Vanilla+ evolution market stock`

### Task 4: Make the six standard evolution stones reusable

**Files:**
- Modify: `src/party_menu.c`

**Interfaces:**
- Produces: `static bool8 IsReusableEvolutionStone(u16 itemId)`; successful item evolution skips `RemoveBagItem` only for Fire/Water/Thunder/Leaf/Moon/Sun Stones.

- [ ] **Step 1: Add `IsReusableEvolutionStone` covering exactly six IDs**
- [ ] **Step 2: Guard the successful-evolution `RemoveBagItem(gSpecialVar_ItemId, 1)` call with `if (!IsReusableEvolutionStone(gSpecialVar_ItemId))`**
- [ ] **Step 3: Commit**

Commit message: `feat: make evolution stones reusable`

### Task 5: Verify GREEN build and artifacts

**Files:**
- Test: `tools/vanillaplus_phase01_verify.py`
- CI: `.github/workflows/build.yml`

**Interfaces:**
- Consumes: all previous tasks.
- Produces: green source verification and green agbcc builds for both release and test configurations.

- [ ] **Step 1: Run source verifier**

Run: `python3 tools/vanillaplus_phase01_verify.py`
Expected: PASS.

- [ ] **Step 2: Run release build**

Run: `make -j$(nproc) all`
Expected: exits 0 and produces `pokeemerald.gba`.

- [ ] **Step 3: Run test build from clean tree**

Run: `make mostlyclean && make -j$(nproc) VANILLAPLUS_TEST=1 all`
Expected: exits 0 and produces `pokeemerald.gba` with test header title.

- [ ] **Step 4: Inspect uploaded artifacts and hashes**

Expected release filename: `pokemon-emerald_v1.gba`.
Expected test filename: `pokemon-emerald_v1-test.gba`.
Both retain game code `BPEE`; release title is `ZUMRUT VP002`; test title is `ZUMRUT T002`.

- [ ] **Step 5: Manual emulator acceptance test**

Using the user’s existing `.srm`, verify Continue → Mart stock → save → close/reopen → Continue. Using the test build, evolution scenarios can be prepared quickly in later test-hook tasks without requiring story progression.
