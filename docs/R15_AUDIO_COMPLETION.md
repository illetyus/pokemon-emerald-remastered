# R15 Audio — source and preparation acceptance

> Current integration status (R20-I5 reconciliation): pre-real-Unreal source
> acceptance and PR/main integration are VERIFIED_COMPLETE. The integration
> ledger in [R8_INPUT_INVENTORY.md](R8_INPUT_INVENTORY.md) supersedes historical
> merge-pending/per-chat authorization statements below. Actual asset, host,
> engine, runtime and device obligations retain their documented limits.

G1: **VERIFIED_COMPLETE** for the canonical pre-real-UE source/preparation scope
at `5a5475103b736ae5d9c8b0abf442238b8edc05be`.
D1: completion evidence recorded here.
F1: independent source reconciliation **PASS** at
`695d01b56b6afdc2d73ad8b066ca12479125549a`; this publication's exact-head CI must
reach terminal success before integration. No real runtime acceptance is implied.
M1/M2: **NOT_STARTED**; dependent phases must integrate in order.

This is the same pre-real-UE acceptance boundary recorded for R6/R7/R9/R14.
It does not certify playable/imported audio, subjective quality, licensed UE/UHT
compilation or Android execution. ROADMAP's four R15 acceptance bullets and
ARCHITECTURE's R18 build boundary determine this gate. Historical preparation
notes listing real host/device work are remaining runtime obligations, not
evidence that those obligations have already passed.

## Source authority and audited scope

Production authority remains `illetyus/pokezumrut-vanillaplus` at
`70db90c9077aed1272e746fc2537d9f12b95a91c`. The source song/cry/fanfare tables,
constants, conversion recipes, mode switch, pitch/release functions and actual
source bytes are audited; renderer and community cry archives are candidate
tools/inputs, never gameplay authority.

R15's implementation/preparation span from verified R14
`4dc33027e738242d5c514caf7818db00e20bcb0f` to `5a547510` contains 10 commits
and 47 changed files. GitHub's complete compare reports no core/vendor changes
and no audio, ROM, save, ZIP or APK payload additions. The inherited R6/R7/R9/R14
chain is separately audited and must merge via each phase's PR/main-CI gate.

## Canonical G1 acceptance

| ROADMAP acceptance before real UE | Result | Actual evidence |
| --- | --- | --- |
| Required semantic audio IDs resolve or fail explicitly | PASS | 609 non-silent song IDs and 386 × 13 cry keys: 5,627 semantic keys. Generated catalog, portable resolver, same-key profile lookup, explicit missing/unvalidated/load errors and invalid-ID regressions. Silence 0/MUS_NONE is explicit. |
| Loop/transition metadata is deterministic | PASS | Byte-exact source catalog, 209 conversion recipes/timelines, all 176 native/rational loop comparisons, fixed authored crossfade policy and bounded PCM loop metadata. Playback readiness is separate and false. |
| Gameplay code has no asset-path dependency | PASS | Core/vendor unchanged; generated/portable identities contain no audio paths. Presentation consumes const committed requests and cannot complete source waits or mutate gameplay/RNG/save state. |
| No copyrighted audio binary is committed | PASS | Public artifacts contain source identities, hashes, provenance, validators and measurements only. Candidate payloads and outputs remain under ignored local/private paths. |

The 80 reserved zero-track rows (270–349) remain historical resolver keys with
explicit missing/unvalidated asset behavior. They are not invented recordings
or an undocumented global music-stop policy. Only IDs 0/65535 map to the
explicit stop sentinel. Source fanfare wait frames remain authoritative; WAV
durations and presentation completion never substitute for those waits.

## Inventory and implementation disposition

| Subphase | Source/preparation disposition |
| --- | --- |
| P1/P2 | Pinned semantic inventory, provenance, private payload boundary and strict manifest/receipt contracts are implemented. |
| I1 | Original/Modern maps use identical keys; exact-key Original fallback is explicit. Missing/unvalidated assets fail with a reason. Egg/old Unown placeholders are unsupported; extended Unown shares cry.201. |
| I2 | 191 real music + 18 jingle jobs rendered privately; 176 measured loop candidates, 33 one-shots. Source volume/voicegroup/reverb are retained. Authored crossfade and validated frame metadata are deterministic; runtime loop assets remain unvalidated. |
| I3 | 269 SFX, 51 bard phonemes, 18 jingles and source weather/ambience SE identities are mapped. 386 normal + 4,632 special cry candidates retain exact mode keys. Missing effects/phonemes are explicit. |
| I4 | Portable 16-voice admission/priority/eviction policy is the same code used by UE. Music plus bounded retiring components is separate. Cry attenuation restores when voices finish. |
| I5 | Four bounded volume settings and Original/Modern preference use GameUserSettings, outside Emerald gameplay saves. New default is Modern; saved user choice is retained. |
| I6 | Background and audio-focus suspension reasons are independent; foreground cannot clear focus loss. Background delegates pause components and NotifyAudioFocus exposes the platform boundary. Actual Android callback attachment remains runtime work. |
| T1/V1 | Source/catalog, strict probe, compiled-source cry/converter oracles, integrity, corruption, timing, policy/lifecycle and authority regressions pass exact-head CI. |

Candidate preparation does not set imported readiness. All 5,018 cry candidates
and all 209 music/jingle candidates are private, technically probed assets.
The latter are Original-style candidates; no Modern BGM pack is asserted.
The 209-entry Original BGM manifest leaves 5,418 historical keys unrepresented.
The cry pack's other 609 song keys remain explicitly missing. Combining these
corpora conceptually covers 5,227 keys, leaving 400 reserved/effect/phoneme keys;
no merged playable/imported profile or enabled fallback is inferred from that sum.

## Actual verification evidence

Exact implementation/preparation HEAD: `5a5475103b736ae5d9c8b0abf442238b8edc05be`.
[Workflow 37779942858](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37779942858)
reached terminal success for both jobs:

- 74/74 targeted Python tests, zero skips;
- 310/310 full Python regressions, zero skips;
- 67/67 CMake/CTest portable tests, including the actual UE-shared resolver/policy;
- exact catalog, cry recipe and BGM plan regeneration checks;
- Unreal authority source guards.

Private BGM preparation required 707/707 pinned renderer engine checks and all
589 input blob/byte/hash receipts. All 209 outputs passed strict PCM probes,
all 176 loop bounds matched native and rational scheduling, and all four prior
pilot WAV hashes replayed exactly. Aggregate BGM PCM is 4,300,978,864 bytes;
this is an offline corpus size, not a resident-memory/device benchmark.
Cry replay matched every historical normal WAV hash and the full special
manifest; changed ZIP container hashes are explicitly distinct from payload
equivalence. Supporting artifacts:

- [Cry recovery](R15_AUDIO_PACK_RECOVERY.md);
- [BGM source contract](R15_BGM_SOURCE_PLAN.md);
- [Native render pilot](R15_BGM_RENDER_PILOT.md);
- [Complete BGM coverage](R15_BGM_RENDER_COVERAGE.md);
- [Historical preparation details](R15_AUDIO_PREPARATION.md).

## Explicit deferred runtime/local asset obligations

These are open and must remain visible through R18 and the real-runtime gate:

1. Audition all candidate assets/seams, record species/mode exceptions and
   compare special modes/BGM with an actual source-game hardware/emulator reference.
   Community archive/poryaaaa outputs are not certified GBA mixer fidelity.
2. Supply/verify private effects, phonemes and any additional Modern music;
   import actual Original/Modern assets and prove exact-key fallback behavior.
3. Author and validate actual intro/loop SoundWave/SoundCue playback. Measured
   PCM bounds alone do not implement a loop; imported/loop readiness stays false.
4. Attach the actual committed-request owner exactly once, authoritative map
   BGM context and source waits. SAVE_BGM/FADE_DEFAULT remain explicitly unsupported
   by the const audio adapter. Do not infer requests by UI/animation polling or
   use playback duration to complete gameplay waits.
5. Attach Android audio-focus callbacks and validate real interruption/resume,
   background transitions, profile/settings UI, memory/loading, latency and device mix.
6. Compile UE/UHT, import/cook/package on the actual R18 project environment;
   then stop at REAL UNREAL RUNTIME VALIDATION as authorized. Source validators
   and portable tests are not substitutes for those environment results.

No physical Android smoke, BrowserStack, final device matrix or R21/R22 is
authorized to run automatically at that stop boundary. Missing actual R18
environment must be reported as EXTERNAL_ENV_REQUIRED.

## Integration sequence

Independent F1 reconciliation reread live main/R15 refs, recent commits, compare,
open PRs, exact-head CI, canonical architecture/phase/roadmap, audio contract and
actual const resolver/consumer source. Main stayed `a7ed422d`, the branch stayed
`695d01b`, and the complete main compare was 29 ahead / 0 behind, 139 changed files,
with no vendor/commercial binary changes or conflicting session drift. G1/D1's
five-file documentation commit left the tested implementation unchanged.
Workflow 37780786306 reached terminal success: 74/74 targeted Python, 310/310
full Python, 67/67 CTest, zero Python skips. Contract, readiness flags, source
pins, special-mode boundaries, reserved row policy and deferred runtime ledger
agree. No remaining pre-real-UE acceptance RED was found.

This final evidence publication remains subject to exact-head CI; only after
that success is R15 source/preparation F1 VERIFIED_COMPLETE and integration allowed.

After independent R15-F1 and its exact-head CI, integrate verified dependent
phases in order **R6 → R7 → R9 → R14 → R15**. Each phase requires a PR merge and
terminal-success CI on the resulting live main before the next merge.
R15-M2 success permits **R8-P1**, then R19, R20 and R18.
