# R15 audio preparation and listening pilot

Status: P1 inventory and P2 source/payload contract implemented. A 12-species
listening pilot and presentation resolver/settings/lifecycle scaffolding exist.
**R15 is not complete.** Modern quality acceptance, the full 386-species pack,
rendered music/FX, cry-mode processing, real host/platform attachment and Unreal /
Android validation remain open. No merge to main is authorized by this step.

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

Listen before approving modern pack expansion. Assess recognizable identity,
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
Cry mode preprocessing/authored engine effect graphs are still missing.

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

1. Listen to the pilot; select global modern direction and species exceptions.
2. Acquire/process all 386 approved cries privately; derive all 13 source modes
   using a verified render or authored equivalent; assess source cry fidelity.
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

## Verified implementation CI

Implementation commit: `8342d1b1fecbc6c2ea3ede63ff6c573bc44cb72b`, sole parent
R14 `4dc33027e738242d5c514caf7818db00e20bcb0f`. All 21 source/document blobs
match the local tested bytes; no core/vendor changes or audio binary additions.
[Implementation workflow 37654758027](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37654758027)
completed successfully. Both jobs succeeded: 20 targeted R15 tests, all 256
Python regressions and all 67 CMake/CTest tests. Local R15 C++ policy checks: 5,746.
This proves source/portable checks, not Unreal/UHT, subjective quality or Android
runtime acceptance. A later documentation-only commit preserves this exact
implementation and its evidence.
