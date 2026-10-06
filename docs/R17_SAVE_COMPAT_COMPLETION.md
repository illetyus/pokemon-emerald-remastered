# R17 — Save compatibility completion evidence

Date: 2026-10-06
State: R17-G1 VERIFIED_COMPLETE; R17-D1 VERIFYING until its exact commit CI succeeds.
Branch: `r17-save-compat-mainline`
PR: [#16](https://github.com/illetyus/pokemon-emerald-remastered/pull/16)
Acceptance/runtime-test HEAD: `f0db6e28a06ebd104f9e184c475a0ec359ca31f6`
Main baseline: `27c9bcfba5bd4e1a0e1eb996fe61e468b6eab6d6`

Final Gate, PR merge and terminal-success main CI are separate checkpoints.
This evidence does not mark them complete in advance. Resolve the latest
checkpoint from this document, commit history, PR checks and live refs together.
After a successful D1 commit check, the next state is R17-F1. After F1, M1 and
M2 pass, the next phase is R6-P1.

## Acceptance matrix

All five ROADMAP R17 acceptance requirements passed at the acceptance HEAD.

| Requirement | Observed evidence |
| --- | --- |
| Representative real saves load | The user's hash-pinned 128 KiB VP019 sample imports through actual portable platform/native read APIs; both slots independently validate, selected checkpoint 72/0/2. Only this supplied real sample is claimed. |
| Save -> remaster -> save remains valid | Actual private export is 73/1/3, then reopens through a new native process. The independent whole-image oracle verifies payloads, footer/checksum/rotation, untouched main slot and all special sectors. |
| Objective, party, bag, world and progression survive | Private replay retains objective 15, six valid party records, all five bag pockets, world/progression and RTC. Public I2/T1 regressions additionally cover flags/vars, storage, saved NPCs and encounter-domain offsets in both compiled formats. Objective remains derived from core state. |
| Corrupt/unsupported input fails safely | I1/I5 cover every required section, ID/signature/checksum/coherence failures, valid backup selection, wrong size, explicit format, future VP5 versions, MISSING vs ERROR, stale checkpoints and failed write/retry. Failure cannot silently create/overwrite an image or advance committed metadata. |
| Compatibility fixtures are versioned and regression-tested | 37 public recipes with pinned hashes and an independent domain/byte oracle; seven existing exhaustive CTest fixture owners/source blobs; private original hash and name-free semantic evidence plus optional local replay. Full suite passes 61/61. |

Detailed recipes, commands and redaction limits:
[R17_SAVE_FIXTURE_MATRIX.md](R17_SAVE_FIXTURE_MATRIX.md).
Format and deliberate policy:
[R17_SAVE_COMPAT_CONTRACT.md](R17_SAVE_COMPAT_CONTRACT.md).

## Production source and compatibility policy

Pinned Vanilla+ authority is
`illetyus/pokezumrut-vanillaplus@70db90c9077aed1272e746fc2537d9f12b95a91c`.
Stock layout authority is
`pret/pokeemerald@731ad5bfd6e6f265508d0efcca0ba42f9dcf5881`.

The production AGBCC MODERN=0 measurements supersede stale header comments:
Vanilla+ SB2/SB1 are 0xF44/0x3DC8; stock is 0xF2C/0x3D88; storage is
0x83D0 with boxes at 0x4. Key offset SB2+0xAC is shared. Production VP5 is
21 packed bytes at Vanilla+ SB1+0x35D8; magic 0x35504956, version 1.
No Emerald block or 128 KiB image was expanded.

- Callers explicitly select stock or Vanilla+; the shared signature and
  ambiguous checksum spans are not format detection.
- The pinned MAX/0 counter exception and ordinary unsigned ordering are
  retained. Equal counters choose physical slot 0. Requiring all 14 counters
  to agree is documented remaster corruption hardening, not a source claim.
- Export uses new counter parity, next rotation and source checksum/footer
  spans. The other main slot and sectors 28-31 remain byte-exact.
- A failed platform write retains the entire caller checkpoint and pending
  edits. Native temporary-file close/replace is tested independently.
- Legacy VP5 recovery requires explicit caller provenance, valid v1 data and
  a uniformly blank production footprint; it neither clears legacy Mystery
  Gift bytes nor invents missing original data. Production metadata wins.
- Stock QoL extensions are a runtime sidecar, never new stock save fields.
  Unsupported VP5 versions are preserved by explicit raw inspection/export
  but rejected by ordinary platform load/store. No automatic layout conversion
  or destructive repair is introduced. Remaster performance preferences remain
  outside Emerald gameplay saves.

## Real fixture identity and native results

Original SHA-256:
`4cf20c4310a49fe4a7e0782f89deaa8e4efa45b2f46a9f443941d506fcd2a633`

Export SHA-256:
`db9bf9a76ace9158aed5ad68469de6c3e08dd7f848c67dc3ea6b6f4d1b05b001`

The original remained unchanged. Strict C99 core/C++17 probe and C/C++
ASan/UBSan each passed 37/37 public recipes and the private real replay.
Leak detection was disabled under ptrace; no LeakSanitizer claim is made.
Seven intentionally wrong test-only probe outputs (world, party, bag, RTC,
objective, payload and special-sector byte) were all rejected by the oracle.

The original private save is not versioned or uploaded. The public projection
uses actual name-free semantics with artificial identity, storage and opaque
values; it is explicitly synthetic. Public CI does not claim access to the
private original. No new emulator, ROM or actual stock-save replay was run.

## Exact acceptance-HEAD CI

All nine runs below are completed/success and report the acceptance HEAD.
PR workflows checked their corresponding GitHub synthetic merge checkout;
the underlying branch was 11 ahead / 0 behind the unchanged main baseline.

| Workflow | Run | Actual relevant result |
| --- | --- | --- |
| R0 Core | [37532617227](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37532617227) | Portable full suite 61/61; Windows read 32/32 and atomic write 24/24; converter/source checks pass |
| R16 Vanilla+ QoL | [37532617237](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37532617237) | 3/3 QoL/source/C++ embed, item/Bag/PC regressions and source architecture pass |
| R5 World Renderer | [37532617406](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37532617406) | Renderer tests and source architecture pass |
| R4 Overworld | [37532617146](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37532617146) | Terminal success |
| R10 Quest Map | [37532617199](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37532617199) | Terminal success |
| R11 Pokemon Party Item | [37532617173](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37532617173) | Terminal success |
| R12 Encounter | [37532617123](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37532617123) | Terminal success |
| R13 Battle | [37532617142](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37532617142) | Terminal success |
| PR CodeQL | [37532613402](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37532613402) | All four language analysis jobs completed/success |

D1 changes only documentation. Its commit requires new exact-head CI before
F1; those runs must be read live rather than inferred from this prior table.

## Checkpoint ledger

| Subphase | Verified commit/evidence |
| --- | --- |
| P1 | Existing-surface recovery/audit, preserved in contract and branch history |
| P2 | Corrected AGBCC source contract at 7782bf26a9961fb6eb4d480721f89b95973f721e |
| I1 | d5d27bdd6db797d04d32a26122e18a76b3206509; sector/format validation |
| I2 | 98cad9ff84124cea41d95c6beba8430e5ff2ff22; full-domain reconstruction |
| I3 | 4f5f35c12dc484c61fa4c282738bd28440311a6c; preservation/transactionality |
| I4 | d58955c4c1e4c48dd243b81b874d60463e40c7cd; metadata/migration boundary |
| I5 and I5-Closure-1 | ac50b51674d5707caa0a2ac1d237d687a0af6d9e; full CI 60/60, Windows 32/32 + 24/24 |
| T1 | f0db6e28a06ebd104f9e184c475a0ec359ca31f6; versioned matrix and real replay, full CI 61/61 |
| V1 | Read-only exact-HEAD full/targeted CI reconciliation; nine terminal-success runs |
| G1 | Read-only acceptance/source/architecture/scope reconciliation at the same HEAD; five requirements pass |
| D1 | This documentation checkpoint; VERIFYING until its own committed HEAD is green |
| F1/M1/M2 | Pending separate live reconciliation, verified PR merge and terminal-success main CI |

Initial I5 commit 825e53f904f4f7211280a341cdd00cc512ca9184 failed Linux
59/60 (directory size probe), despite Windows passing. It was explicitly
NEEDS_CLOSURE. Closure-1 requires a regular opened descriptor before sizing;
the subsequent complete green run closed the gap. The failed commit was not
accepted as phase completion.

## Limits and deferred gates

Native/core compatibility is verified. Shared native reader/atomic-writer
tests and Unreal source checks do not prove real UE compile, cook, packaging,
Android filesystem/lifecycle behavior or device runtime. Those remain R18 and
the explicit REAL UNREAL RUNTIME VALIDATION stop. No physical Android smoke,
BrowserStack, final device matrix, R21 or R22 is authorized to run automatically
beyond that stop. Cross-process locking/CAS after a store read is not promised.
