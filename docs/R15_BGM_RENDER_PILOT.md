# R15-I2-Closure-2 — pinned source render pilot

State: VERIFYING; published-head CI remains required.
Parent: `4f8ef9a9769b0f4211e129c80e0dc6ae8181e72e`.
Source: Vanilla+ `70db90c9077aed1272e746fc2537d9f12b95a91c`.
Renderer: [poryaaaa](https://github.com/huderlem/poryaaaa/tree/4000591de6c397b6c80adc07af17144e26b30dfd)
`4000591de6c397b6c80adc07af17144e26b30dfd`.

## Actual preparation result

Four original-style private render candidates passed the local pipeline.
All are stereo PCM16 at 48 kHz. Each has zero full-scale samples and nonzero
decoded signal. These technical checks do not approve perceived quality.

| Semantic key | Source music | Decoded frames | Intro/loop frame range |
|---|---|---:|---|
| song.367 | mus_level_up | 183310 | one-shot plus 144000-frame renderer tail |
| song.405 | mus_littleroot | 7719991 | 40000–2599997 |
| song.423 | mus_encounter_suspicious | 2714751 | 165246–1015081 |
| song.474 | mus_vs_wild | 6288969 | 646530–2527343 |

The three music renders contain intro plus exactly three loop-body repetitions,
with no preview fadeout. Their frame counts match both an independently computed
rational MIDI timeline and the pinned CLI's actual `parse_midi` output. The
small native oracle includes the unmodified CLI under a renamed main function;
its source hash is recorded. This proves bounds for these candidate renders,
not GBA timing equivalence or Unreal SoundCue loop playback.

`mus_encounter_suspicious` retains the source CC7=128 values. Song master volume,
voicegroup and reverb come from the source recipes. Polyphony is 5, PCM mix rate
13379 Hz, output rate 48000 Hz. Analog-filter/base-key/portamento/PWM opt-ins are
disabled. Those controls, nominal MIDI scheduling and hardware-timing differences
remain explicit fidelity work; these outputs are not certified hardware renders.

## Inputs, transport and reproducibility

The private source snapshot has 384 verified files: 380 source bank/sample/table
inputs and four MIDI files. The renderer snapshot has 13 verified source/header
files, including its miniaudio dependency. Every receipt records byte size,
Git blob SHA-1 and SHA-256. No source files were edited to obtain the renders.

The 25 programmable-wave files total 400 bytes. Contents-tool responses altered
some non-UTF-8 bytes. Size/Git/SHA checks rejected those copies. Authorized
Composio GitHub raw downloadable copies preserved the exact original blobs.
Only verified bytes reached the renderer. Signed URLs and credentials are not
included in public evidence.

`tools/build_r15_bgm_pilot.py` checks all snapshots, compiles the pinned renderer,
its native MIDI oracle and its engine tests, and requires **707/707** engine
checks before rendering. Source compilation uses GNU C11 and `-O2`; this matches
the upstream CMake C11/extensions policy without installing its GUI dependencies.
Replays bind to the versioned snapshot receipts. Changing a file and its local
claimed hashes cannot silently replace the pinned baseline.

The manual pilot and a fresh build through this tool produced identical SHA-256
values for all four WAVs. The existing strict local audio-pack probe also passed
all four candidates; its report retains 5623 unrepresented historical keys.

```
python tools/build_r15_bgm_pilot.py --sources /private/pinned-source --renderer /private/pinned-poryaaaa --output /private/new-empty-pilot
python tools/validate_r15_audio_pack.py /private/new-empty-pilot/manifest.json --root /private/new-empty-pilot
python -m unittest discover -s tests -p 'test_r15_bgm_pilot.py' -v
```

Public evidence: `data/r15/bgm_pilot_evidence.json`, metadata/hashes only.
MIDI/AIFF/PCM/WAV and compiled binaries remain private and outside commits.

## Tests and gate boundary

Regression-first RED failed before the module/evidence existed. Local GREEN:
12/12 new regressions passed, including exact arbitrary-byte transport,
false receipts, snapshot pin/path/symlink failures, tempo boundaries/rounding,
ambiguous/mismatched native loops, silent/clipped/truncated/wrong-shape WAVs,
all 384 source receipt comparisons and false acceptance promotion. The partial
local source snapshot was hydrated with verified original bytes for those
receipt checks; the complete hosted checkout remains the full-suite gate.

`loop_ready`, seam listening, runtime-loop, quality, hardware equivalence and
Unreal import acceptance remain false. The local candidate manifest uses
`loop=null`; measured loop evidence does not enable runtime looping by itself.
Source fanfare waits and gameplay authority remain untouched.

On published-head CI terminal success, next named subphase:
**R15-I2-Closure-3 — remaining music/jingle render coverage**.
205 source jobs remain; SFX/phoneme/ambience, owner attachment and other R15
acceptance gaps also remain. No R15 G1/F1/merge or real Unreal/Android acceptance
is claimed by this four-file pilot.
