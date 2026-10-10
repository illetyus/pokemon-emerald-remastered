# R20 — Named implementation evidence

## R20-P1 — Production-tree audit

VERIFIED_COMPLETE at a7db2bbb07e2cfdbf0021db9949b3c6de8007881.
Only four docs changed; live refs/compare, exact published strings/blobs and
scope were verified. Source inventory: 524 non-vendor blobs, 270 relevant source
blobs verified, complete module reference search. Baseline main all 16 workflows
succeeded; actual 81 CTest / 400 Python / 171 asset audits. PR #24 is draft.
Inventory and explicit I1-I7 gaps: R20_PRODUCTION_INVENTORY.md.
P1 exact-head PR workflow 38013904950 / job 114099865660 completed/success:
actual 81 CTest, 400 Python, 171 asset audits and all seven source components.
All 16 exact-head workflows succeeded before I1 publication.

## R20-I1 — Historical presentation excluded from production

State: VERIFYING publication/source CI.
Before: unused R0HUD and R0PlayerController UCLASS pairs lived in unreal/Source
and were compiled/scanned by the production Unreal module despite having no
consumer outside their own pairs. The configured R0GameMode already selected
RemasterPlayerController/RemasterOverworldPawn and HUDClass nullptr.

Change: move four files intact to experiments/unreal-r0-reference, outside UBT/
UHT discovery. Preserve the active R0GameMode, RemasterCoreSubsystem platform/
save bootstrap, all portable C/C++ authority/fixtures and existing historical
SDL/Godot experiments. No gameplay, persistence or live input behavior changes.

Original/preserved Git blobs:
- R0HUD.h: 8ede80c6a433ac8e6f143d4f69c0ef310e9acaeb
- R0HUD.cpp: 668b912bf1f6c87eae8272826b074aeaeae4ab02
- R0PlayerController.h: 68ba3fe96735ba5250696a937d5beadceef9bd50
- R0PlayerController.cpp: 49493b54e4711c1f5f15b1467869f8d090d670cf

Source boundary regression: the existing Unreal architecture validator now
rejects any of the four historical files inside the runtime module and requires
the preserved reference paths. Before the move it failed with all four expected
messages (RED). After the move it passed; all four old/new byte sequences and
Git hashes are identical (GREEN). No new implementation-mirroring test was added
for this reversible move. Actual full portable/source CI remains mandatory.

Production module dependency/embedded-core and default R4/R8/R9 host guards
continue to pass locally. No real UE/UHT compile or private cooked asset
acceptance is claimed; that remains R18/runtime evidence.

Next: verify this exact published HEAD's R19 full-suite and earlier-phase source
guards, then R20-I2. M1/M2 remain NOT_STARTED.
