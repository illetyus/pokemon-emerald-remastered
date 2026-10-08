# R15 cry pack recovery — 2026-10-08

Current subphase: **R15-I3-Closure-1 — hash-pinned cry pack recovery**.
Local recovery and replay checks passed. Published-head CI is a separate gate.
R15 as a whole remains incomplete.

## Repository search

The user requested a search in both GitHub repositories before any rebuilding.

- Production main: `a7ed422da7817c05aa1eac21bdbca6bd6edced5e`.
- Production preparation parent: `3b9b208f934ecba9ac9be857cd40e38e88cbfec0`.
- Private asset main: `75303f4e49de60015d8ffcd28a56b6195d7cb119`.

The private asset tree was complete (126 entries, 108 blobs), with one branch
and no releases. Its ZIPs are R6 character/NPC sources; there are no R15 packs.
The production preparation directories were enumerated separately to avoid the
oversized full vendor tree. They contain R15 tools and evidence, but no prepared
audio payloads. Production has no releases or additional audio phase branch.
The vendored sound/download directories contain legacy source material, not the
386 modern normal/special ZIPs. The session's local/Library-file directories also
had no existing cry archives. This establishes current availability, not deletion
of any earlier user Library copy.

## Recovery and unchanged content

Existing versioned tools were used to recover unavailable local output. All 386
OGG masters were fetched from `PokeAPI/cries@ef687b18f0ce17169b4b4c09175819f7ade92f0f`.
Each returned Git blob SHA, decoded byte length and source SHA256 matched the
historical per-species receipt before conversion.

The normal builder used FFmpeg 7.1.5. **All 386 normal WAV hashes and frame counts
match the historical records exactly.** All 4,632 special candidates were then
rebuilt using the unchanged pinned recipe and these unchanged normal WAVs.

The complete special manifest matches the historical SHA256:
`1e0cb80a72723a8bc0aedbdbd4962854d6f3948543b47d089b97a737566696d8`.
Its 5,018 candidate payloads passed the strict hash/PCM/provenance probe. All
386 special-pack master copies also match historical source receipts. The other
609 semantic keys remain explicitly missing.

The 777-member normal ZIP and 5,412-member special ZIP passed duplicate/path/
symlink/member-coverage checks, full CRC reads and byte comparisons with every
actual private pack file. Their containers are newly written, so their ZIP hashes
differ from the original containers. Historical evidence was preserved unchanged.

| Pack | Recovered bytes | Recovered SHA256 |
| --- | ---: | --- |
| Normal | 34117563 | `a0f1ca22176a7be1a653205a6270ba4cf000c10b1ef85b55070a785c2ec799f6` |
| Normal + special | 311477953 | `6cfa7a4c99b71051601b00d00dff7b9a7d76f2a668f12363545c4447f345376b` |

Public metadata receipt: [cry_recovery_evidence.json](../data/r15/cry_recovery_evidence.json).
Commercial payloads remain private/ignored; the public receipt contains no audio.

## Reproduction

Source receipts must contain the exact pinned `commit`, Git blob `sha` and
`size` for Dex 1..386. Existing fetch/build tools validate those bytes:

```sh
python tools/fetch_r15_modern_cries.py --receipts RECEIPTS.json --output local/r15/recovery_sources
python tools/build_r15_modern_pack.py --source local/r15/recovery_sources --output local/r15/recovered_normal --public-evidence local/r15/recovered_normal_evidence.json
python tools/build_r15_special_cries.py --normal-pack local/r15/recovered_normal --output local/r15/recovered_special --public-evidence local/r15/recovered_special_evidence.json
python tools/validate_r15_cry_recovery.py --normal-pack local/r15/recovered_normal --special-pack local/r15/recovered_special --normal-archive local/r15/Pokemon_386_Modern_Ses_Paketi.zip --special-archive local/r15/Pokemon_386_Ozel_Ses_Adaylari.zip --report local/r15/cry_recovery_audit.json
```

The recovery verifier fails when historical normal/master identities differ,
the special manifest/recipe differs, or archive bytes/members differ from the
validated private packs. A rebuilt container is explicitly reported separately
from historical payload/manifest equality.

Local tests: eight original synthetic regressions passed for valid archive
replay, duplicate members, path escape/symlink, altered bytes with valid ZIP CRC,
missing/unexpected members, historical identity, changed normal and changed
master rejection. The actual private replay above also passed.

## Previous CI closure

**R15-V1-Closure-1 is VERIFIED_COMPLETE** at
`3b9b208f934ecba9ac9be857cd40e38e88cbfec0`.
[Workflow 37763598815](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37763598815)
completed successfully: 35/35 targeted Python, 271/271 full Python, 67/67 CTest.
The required pitch/duration/reverse fixture executed in both Python suites;
zero tests were skipped. Hosted FFmpeg/FFprobe were 6.1.1-3ubuntu5.

## Remaining gates and next state

Recovered candidates retain `individual_listening_verified=false`,
`hardware_audio_equivalence_verified=false` and
`unreal_import_validated=false`. Hash replay does not approve those gates.

After this publication's exact HEAD CI succeeds, proceed to **R15-I2 —
BGM/loop/transition metadata**. Special-mode listening/reference, the 609 other
audio semantics, runtime-owner dispatch, Android focus/lifecycle and real
engine/device checks remain open. R15-G1/D1/F1/M1/M2 have not passed. Existing
R6/R7/R9/R14 source/preparation acceptance remains separate from their merge/main
gates. R18 main verification still stops at REAL UNREAL RUNTIME VALIDATION.
