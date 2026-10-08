# R15-I2-Closure-3 — full source music/jingle render coverage

State: VERIFYING; exact published-head CI is required.
Parent: `2ae0e2602ce3e01479ddd748bbce84e26059ffd9`.
Vanilla+ source: `70db90c9077aed1272e746fc2537d9f12b95a91c`.
Poryaaaa renderer: `4000591de6c397b6c80adc07af17144e26b30dfd`.

## Scope and reproducible pipeline

`tools/build_r15_bgm_coverage.py` renders every actual music/jingle job in
`bgm_source_plan.json`: 191 music jobs and 18 jingles. The 80 zero-track reserved
song rows are excluded explicitly; no invented sound file replaces them.
Historical resolver/catalog identities and cry pack hashes are unchanged.

The source snapshot has exactly 589 inputs: the pilot's 380 unchanged bank,
sample and table files plus all 209 source MIDI files. Bank receipts bind to
the versioned pilot and MIDI hashes bind to the source plan. The renderer's
13 source/header files bind to the same pilot receipts. Size, Git blob SHA-1,
SHA-256, pin, coverage and path checks run before any compilation/rendering.
No vendor source or renderer source is edited.

The shared pipeline compiles the exact pinned CLI, its native MIDI oracle and
engine tests, and requires 707/707 native engine checks. All renders retain
source volume, reverb, voicegroup and source MIDI bytes; output is stereo PCM16
at 48 kHz, PCM mix rate 13379 Hz and polyphony 5. Loops contain intro plus three
body repetitions without fadeout; one-shots have a three-second renderer tail.
Analog/base-key/portamento/PWM opt-ins remain disabled.

Every actual output must be nonempty, nonsilent, correctly framed and free of
full-scale PCM samples. The native loop parser and independent rational MIDI
tempo timeline must agree on every loop frame bound and output repeat length.
All four previous pilot WAV hashes, settings and scheduling receipts must
replay exactly within the full run. A missing input, warning, renderer failure,
decode failure or frame disagreement aborts the pipeline; no complete evidence
file is published for a failed run.

```
python tools/build_r15_bgm_coverage.py --sources /private/pinned-source --renderer /private/pinned-poryaaaa --output /private/new-empty-full-bgm --public-evidence data/r15/bgm_render_coverage.json
python tools/validate_r15_audio_pack.py /private/new-empty-full-bgm/manifest.json --root /private/new-empty-full-bgm
python -m unittest discover -s tests -p 'test_r15_bgm*.py' -v
```

Public evidence contains metadata and hashes only. Actual MIDI/AIFF/PCM/WAV,
compiled renderer binaries and render logs remain private/ignored. This is an
original-style candidate profile; it does not claim a modern music pack.

## Gate limits and next state

Actual local result: **209/209** renders, **176** loop candidates and **33**
one-shots (15 music and 18 jingles); zero remaining music/jingle jobs. All four
pilot WAV SHA-256 values repeat exactly. All 209 strict candidate probes pass;
that Original-only manifest has 5,418 unrepresented historical song/cry keys.
It is not a complete imported gameplay audio profile.

Aggregate output is 1,075,244,716 decoded frames / 4,300,978,864 PCM bytes,
with maximum absolute sample 17,407 and zero full-scale samples. The largest
WAV is 60,525,044 bytes, within the existing 64 MiB preimport per-file budget.
Aggregate bytes are an offline candidate corpus size, not resident-memory or
device-performance evidence.

Regression-first RED failed before the coverage tool existed. Local GREEN:
6/6 new coverage regressions and 31/31 combined BGM tests passed, including all
209 compiled pinned MIDI conversion recipes. Regressions reject missing,
duplicate/reserved identities, wrong bank/MIDI/renderer receipts, altered
recipes/settings/timing, changed pilot hashes, silent/clipped/incorrect frames,
incomplete engine checks and false runtime/quality promotion. All 589 public
input receipts also match actual immutable pinned source bytes. Published-head
CI remains required for the checkpoint to become VERIFIED_COMPLETE.

The private candidate manifest keeps `loop=null`, and every render receipt has
`loop_ready=false`. Measured boundaries do not implement an Unreal intro/loop
SoundCue. Quality, seam listening, runtime-loop verification, hardware audio
equivalence and actual Unreal import gates remain false. Native engine tests
and native parser agreement do not prove GBA hardware fidelity.

After this checkpoint's exact-head CI terminal success, proceed to
**R15-G1 — pre-real-UE acceptance audit**. Audit the four canonical ROADMAP
acceptance bullets, enter a named closure for any source gap, and reconcile
actual runtime/asset gaps separately against the R18 validation boundary.
No R15 final gate or merge acceptance is inferred from render count alone.
