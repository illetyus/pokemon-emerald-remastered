# R15 audio preparation and listening pilot

Status: P1 inventory and P2 source/payload contract implemented. A 12-species
listening pilot and presentation resolver/settings/lifecycle scaffolding exist.
**R15 is not complete.** The modern direction was selected after the pilot.
The 386 normal cries and 4,632 special-mode candidates are prepared. Music/FX,
special-mode listening and hardware comparison, real host/platform attachment
and Unreal / Android validation remain open. R15 merge remains blocked by its open final gate.

## References and pins

- Parent: R14 `4dc33027e738242d5c514caf7818db00e20bcb0f`.
- Source: Vanilla+ `70db90c9077aed1272e746fc2537d9f12b95a91c`.
- Candidate cries: [PokeAPI/cries](https://github.com/PokeAPI/cries), commit
  `ef687b18f0ce17169b4b4c09175819f7ade92f0f`, `cries/pokemon/latest/<dex>.ogg`.
  Latest is a community archive candidate, not a guarantee of one game/version.
- Music render candidates: [poryaaaa](https://github.com/huderlem/poryaaaa)
  (`4000591de6c397b6c80adc07af17144e26b30dfd`) and
  [agbplay](https://github.com/ipatix/agbplay)
  (`0b87da48d2502da359e45718eec8566ac40fa9d7`). Neither was installed/run here.
- AI enhancement is optional and offline; no model was installed or applied.

The public branch stores source identities, tools, hashes and audit metadata.
Commercial OGG/AIFF/WAV, HTML containing audio and the pilot ZIP are local/private
user deliverables. Repository CC0 does not grant rights to Pokemon audio.

## Source inventory

`python tools/build_r15_audio_catalog.py --check` regenerates and compares exact
source catalogs. There are 610 song table rows: 271 music, 269 SFX, 18 jingles,
51 bard phonemes, one silence row. All 386 species map to the actual Emerald cry
table and AIFF source hash; 13 source cry-mode IDs and 18 fanfare indices/waits
are recorded. MUS_DESERT is a source constant alias, not a fabricated file name.
Species/core IDs and National Dex IDs differ. Extended Unown 413..439 share
cry.201; old Unown placeholders and Egg fail explicitly. Shiny and visual forms
do not generate additional cry recordings.

Resolver keys: `song.<source_id>` and `cry.<dex>.mode.<0..12>`. 0 and MUS_NONE
65535 stop music. The bounded resolver covers 5,627 non-silent song/cry mode
semantics; this is mapping coverage, not 5,627 available recordings.

## Listening pilot

Species: Charizard, Pikachu, Gengar, Electabuzz, Mewtwo, Unown, Treecko,
Gardevoir, Aggron, Castform, Rayquaza, Deoxys. Per species, A is Emerald source
PCM and B is the modern archive candidate. A is **not** certified complete GBA
mixer/hardware output. It is not substituted with an unverified legacy archive.

All 12 modern files were fetched at the pinned commit and their bytes checked
against returned Git blob SHA/length receipts. The 12 Emerald AIFF hashes
reconcile pinned project sources. Masters remain unchanged. Conversion uses
recorded ffmpeg 16-bit mono WAV at 48 kHz, fixed -6.02 dB conversion headroom and
no dithering. The listening copies target RMS 0.06 with -3 dBFS peak ceiling and
4x gain cap. There is no trimming, pitch shifting, denoising or AI. Higher sample
rate is not labelled recovered detail. All pairs matched RMS within 0.001 dB;
final converted/comparison files contain no full-scale samples. This does not
prove the source recordings are perceptually free from distortion.

The combined WAV is 48.7001875 seconds: A/B separated by 0.65 seconds, species
pairs by 1.25 seconds. The ZIP contains masters, 24 decoded WAVs, 24 listening
copies, offline embedded-audio HTML, timing guide, provenance and probe reports.
`data/r15/pilot_evidence.json` contains metadata and hashes only.

Reproduce after fetching exactly the pinned modern source files into ignored
`local/r15/pilot_sources/` and storing actual Contents API receipts as
`receipts.json` keyed by decimal Dex string, with `commit`, `sha` and `size`:

```
python tools/build_r15_audio_pilot.py --modern local/r15/pilot_sources --output /absolute/private/new-empty-folder
```

The user approved the modern sound direction on 2026-10-07 and authorized the
386-species normal-cry expansion. Individual full-pack listening is not claimed.
Assess recognizable identity,
timbre, attack, duration, noise and unwanted new tones. Handle exceptions per
species; 386 species are not presumed approved from twelve metadata probes.

## Presentation infrastructure

`RemasterAudioRead.h` is the same portable resolver/policy used in Unreal.
It has a 16-voice budget with deterministic oldest-lowest-priority eviction;
equal/lower priority arrivals fail explicitly. Focus loss and background are
independent suspension reasons. Clearing one cannot erase the other. Pack reset
removes voices without clearing lifecycle suspension.

`URemasterAudioProfile` has Original and Modern maps with identical semantic
keys. Imported readiness and loop readiness default false. Missing assets return
failure with an explanation; an optional Original fallback uses the exact same
semantic key. There is no silent normal-mode substitute for reversed/faint/etc.
All 12 special-mode keys now have locally authored candidates. Their source
parameter mapping is tested; listening and GBA hardware equivalence are not approved.

The UE subsystem provides music transitions, bounded retiring components,
SFX/jingles/cry/ambience dispatch, four volume controls and pack choice saved to
GameUserSettings rather than gameplay saves. A ticker removes finished voices
and restores cry music attenuation. Background delegates pause components;
`NotifyAudioFocus` is the Android owner's attachment surface. Loop metadata is
validated as PCM frame bounds, but this code does not synthesize intro/loop
playback: locally authored SoundWave/SoundCue implementation must be verified.

Source host integration: the runtime owner may call `PresentCoreRequest` exactly
once for a committed request. It is a const adapter and never acknowledges a
script wait, runs RNG, resolves a battle or mutates a save. Runtime instruction
`.a` becomes `request.local_id`; `.b` becomes `value_u16` (BGM save flag).
PLAY_FANFARE carries a song ID, not the internal FANFARE_* index. WAIT_SOUND and
WAIT_FANFARE remain owner-owned. SAVE_BGM / FADE_DEFAULT require authoritative
map-owner context and return unsupported here. No automatic VM/event polling
attachment or actual Android JNI focus bridge is claimed.

## Local probe and tests

`validate_r15_audio_pack.py MANIFEST --root ROOT` validates local candidate
PCM16/48k shape, frames, hashes, bounded gains/fades, provenance, supported keys
and loop frame bounds. Duplicate JSON keys, nonfinite values, unknown semantics,
absolute/escaping/symlink paths and false promotion to approved status fail.
Reports always have quality_approved=false and unreal_import_validated=false.
Incomplete manifests show unrepresented semantic counts.

Local validation: 20 R15 Python tests; 5,746 portable resolver/policy checks,
compiled with C++17, -Wall -Wextra -Wpedantic -Werror. All 256 Python regressions passed locally. GitHub CI evidence is appended only
after terminal success. Real UE/UHT has not run.

## Remaining work / gates

1. Modern direction selected and 386 normal cries prepared. Retain any
   subsequently reported species exceptions.
2. All 13 source modes now have normal/special candidates. Compare the authored
   special modes with a source-game hardware/emulator reference and audition them.
3. Render 271 music records plus source effects/jingles as needed; measure exact
   intro/loop frames and check seams/context mix. Preserve source fanfare waits.
4. Attach owner dispatch once per source request, source wait/fanfare/BGM
   transition semantics and Android real focus callbacks. No playback-length
   shortcuts that change gameplay scheduling.
5. Import/cook privately, verify actual loop assets and profile/settings UI,
   latency, interruption/resume, decoded memory and device audio.
6. Reconcile R15 G1/F1 and request the separately authorized merge boundary.

P1/P2: implemented. I1/I4/I5: source scaffolding implemented and portable tests.
I2/I3/I6: partial; assets/rendering/mode and host/platform gates remain.
T1/V1: targeted tests implemented. G1/D1/F1/M1/M2: incomplete.

## Prior preparation implementation CI

Implementation commit: `8342d1b1fecbc6c2ea3ede63ff6c573bc44cb72b`, sole parent
R14 `4dc33027e738242d5c514caf7818db00e20bcb0f`. All 21 source/document blobs
match the local tested bytes; no core/vendor changes or audio binary additions.
[Implementation workflow 37654758027](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37654758027)
completed successfully. Both jobs succeeded: 20 targeted R15 tests, all 256
Python regressions and all 67 CMake/CTest tests. Local R15 C++ policy checks: 5,746.
This proves source/portable checks, not Unreal/UHT, subjective quality or Android
runtime acceptance. A later documentation-only commit preserves this exact
implementation and its evidence.

## Approved modern direction and 386 normal cries

On 2026-10-07 the user preferred the modern pilot. `modern_selection.json`
records this style choice separately from individual-file listening or Unreal
import approval. New audio settings default to Modern; saved explicit Original
choices remain respected and both profile maps remain available.

All 386 OGG files were read at the same pinned commit. GitHub tree and file-content
receipts cover exactly Dex 1..386; every locally transferred byte stream matches
its Git blob SHA and byte length. Content and PCM SHA256 hashes are retained.
386 normal cries are prepared in PCM16/48k/mono with the selected pilot's gain
policy, preserved duration and masters. There are no missing normal cries and
no full-scale PCM samples in the prepared files. Individual listening of all
386 files is not claimed.

The private ZIP contains `masters/`, `normal/`, complete semantic manifest,
source receipts, local probe, measurements and a use guide. The manifest has
5,627 known semantics: 386 normal cry candidates and 5,241 explicit missing
records (4,632 other cry modes and 609 song/effect/jingle/phoneme records). It
does not copy normal cries into special modes. All engine readiness flags remain
false. Public `modern_pack_evidence.json` contains metadata only.

Reproduce using a complete pinned-tree receipt dictionary (decimal Dex keys,
commit/SHA/size), followed by:

```
python tools/fetch_r15_modern_cries.py --receipts RECEIPTS.json --output local/r15/modern_sources_386
python tools/build_r15_modern_pack.py --source local/r15/modern_sources_386 --output /absolute/private/empty-folder --public-evidence data/r15/modern_pack_evidence.json
python tools/build_r15_audio_catalog.py --check
```

Measured aggregate normal PCM is 33,866,250 bytes (352.7734375 seconds), not a
resident-memory benchmark. Actual import, per-device loading/cache limits,
source special-mode fidelity, other audio assets and host/platform attachment
remain open. Source regressions include five additional selection/receipt/
coverage tests.

## Source-derived special cry candidates

`tools/build_r15_cry_modes.py` extracts the 13 source modes from pinned
`PlayCryInternal`. Weak doubles retains the source switch fallthrough. Reverse
mode uses the same sample identity in the reverse table; normal/shiny visual
forms remain shared. `SetPokemonCryPitch`, `SetPokemonCryChorus`, `ply_tune`,
`TrkVolPitSet` and the integer MIDI frequency tables determine both rendered
track rates. The raw pitch number is not divided by 15360 to invent a rate.
Chorus 192 is a signed byte with 7-bit tuning wrap, not a 192 ms echo delay.

The independent test compiles the actual pinned switch, setters, track pitch
and MIDI frequency functions with allocation/device-only stubs. All 13 modes
are compared with the Python recipes. It does not emulate the full mixer.
The default tempo is one tick per GBA frame; the source documents a clock of
16,777,216 cycles/second and 280,896 cycles/frame. Gate timing derives from this
clock, not an assumed 60 Hz or a new audio duration.

`tools/build_r15_special_cries.py` verifies every input normal WAV against its
manifest and every original OGG against its pinned Git blob receipt. Normal
WAVs and masters are copied unchanged. Special recordings reverse the modern
sample where requested, change rate using FFmpeg `asetrate` + SWR resampling,
sum both source-tuned chorus tracks, apply the source volume ratio and a
255-level integer release recurrence. Gain is interpolated over each release
frame as an authored click-reduction choice. Necessary attenuation limits mixed
peaks to -3 dBFS; special modes are not normalized back to equal RMS. No AI,
generic delay echo, extra looping or fabricated source samples are applied.

The private pack contains **5,018 cry candidates**: 386 unchanged normal and
4,632 special (386 x 12). Every exact cry key is represented. The other 609
song/effect/jingle/phoneme keys remain explicitly missing; total semantic keys
remain 5,627. All prepared special WAVs pass the strict local shape/hash probe
and have zero full-scale samples. `special_pack_evidence.json` stores compact
counts, recipe/manifest/archive hashes and per-mode sizes publicly. The private
archive includes per-file hashes/measurements and preserved masters. A separate
12-species listening WAV presents normal, weak, faint, Roar 1/2, Growl 1/2.

These are **source-parameter-derived authored modern candidates**, not verified
GBA hardware output. The modern source waveform already differs from Emerald.
The compressed decoder, integer mixer, reverb and precise frame-phase behavior
are not reproduced by the authored renderer. Source hardware/emulator A/B,
individual special-mode listening and real Unreal/Android import remain open.
Preparing an exact semantic key does not pass those quality gates or complete R15.

Reproduce from the existing private normal pack:

```
python tools/build_r15_cry_modes.py --check
python tools/build_r15_special_cries.py --normal-pack /absolute/private/normal-pack --output /absolute/private/new-empty-folder --public-evidence data/r15/special_pack_evidence.json
python tools/build_r15_audio_catalog.py
```

Ten added regressions cover compiled-source parameter/pitch agreement,
signed chorus wrap, weak-double fallthrough, reverse playback, changed duration,
integer release, retained volume ratios, clipping guard, complete semantics and
rejection of missing/mismatched special recipes, base-normal references or invented loops.
The source catalog reports special preparation separately from hardware fidelity
and engine import. All approval/import flags remain false for special candidates.

Local validation after this expansion: all 271 Python regressions passed,
including 35 R15 tests. The complete private manifest independently re-probed
5,018 candidates and 609 explicit missing entries with zero unrepresented keys.
All 386 normal WAVs and OGG masters match the previous pack byte for byte.
The 5,412-member ZIP passes CRC, duplicate-path and archive/manifest/recipe hash
checks. GitHub CI must reach terminal success for the exact published head;
none of these local checks is a real Unreal/UHT or Android test.

## R15-V1-Closure-1 — required FFmpeg transform fixture

Baseline [workflow 37735220892](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37735220892)
at `5deb0a6f8c028e48f73b0c2acbd78432b69f7dcf` completed successfully, but
`test_pitch_changes_duration_and_reverse_order` was skipped because FFmpeg was
absent. The targeted result was 34 passed / 1 skipped out of 35, and the full
Python result was 270 passed / 1 skipped out of 271. CMake/CTest passed 67/67.

The R15 source/catalog job installs FFmpeg and verifies FFmpeg/FFprobe before
either Python suite. It sets `REMASTER_R15_REQUIRE_FFMPEG=1`, so the asymmetric
synthetic waveform test fails explicitly if FFmpeg is missing. Optional local
runs without that requirement may still report the missing tool as a skip.

Local closure verification passed nine selected special-cry regressions,
including the actual pinned-source C oracle and real pitch/duration/reverse
transforms under FFmpeg 7.1.5. All seven recipe source hashes matched the pinned
production bytes, and exact recipe regeneration passed. The missing-tool checks
confirmed an explicit failure in required mode and a skip in optional mode.
The full source catalog and full portable suite remain hosted-CI checks.

**VERIFIED_COMPLETE** at `3b9b208f934ecba9ac9be857cd40e38e88cbfec0`:
[workflow 37763598815](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37763598815)
reached terminal success. Both Python suites executed the required transform:
35/35 targeted and 271/271 full Python passed with zero skips, plus 67/67 CTest.
Hosted FFmpeg/FFprobe were 6.1.1-3ubuntu5. The remaining R15 asset/listening/host/
runtime gates above are still open.

## R15-I3-Closure-1 — recovered cry payload replay

The current GitHub asset/production repositories had no prepared audio ZIPs.
Using the existing versioned tools and pinned source receipts, local recovery
reproduced all 386 normal WAV hashes and the historical full special manifest.
All 5,018 candidates and both complete ZIP contents passed the strict private
replay. Rebuilt archive containers have new hashes; original production evidence
was preserved. Details, current metadata receipt, reproduction and remaining
quality/engine gates: [R15_AUDIO_PACK_RECOVERY.md](R15_AUDIO_PACK_RECOVERY.md).
Published-head CI remains required before closing this checkpoint.
