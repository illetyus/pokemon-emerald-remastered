# R15-I2-Closure-1 — source render and loop plan

State: VERIFIED_COMPLETE at `4f8ef9a9769b0f4211e129c80e0dc6ae8181e72e`.
Exact-head workflow 37769695700 reached terminal success: 56/56 targeted Python,
292/292 full Python and 67/67 CTest; zero Python skips.
Parent: `9df9fae1d56420259f9d48ab950183800a5eea06`.
Authority: Vanilla+ `70db90c9077aed1272e746fc2537d9f12b95a91c` production
`sound/song_table.inc`, `songs.mk`, `tools/mid2agb/*`, and `src/m4a.c`.

## Render inventory and compatibility

The historical catalog has 610 table rows and labels 271 of them as music.
That classification includes 80 reserved IDs, 270–349. Their actual shared
`dummy_song_header` ends `sound/song_table.inc` with `.byte 0, 0, 0, 0`:
zero tracks, blocks, priority and reverb. These rows need no music recording.
`MPlayStart` starts/stops the selected player's tracks under its source priority
rules. Host behavior for reserved requests remains an explicit integration gap;
this checkpoint does not silently turn every reserved request into global stop.

The render plan contains **191 music jobs and 18 jingle jobs**, all backed by
actual pinned MIDI files and conversion rules. 176 music jobs have source loop
markers; 15 music jobs and all 18 jingles have no source loop markers.
Source fanfare waits remain gameplay scheduling data, separate from recording
duration. Each job carries the existing semantic key, player/group, source hash,
voicegroup, master volume, priority, reverb and conversion flags.

`data/r15/bgm_source_plan.json` is additive. It preserves the historical
610-row catalog, 5,627 resolver keys and recovered cry manifests/hashes. Their
609 missing song keys still describe historical pack coverage, not 609 sounds
that must be fabricated. No core, vendor or Unreal playback behavior changes.

## MIDI timeline contract

The pinned converter's `ReadSeqEvents` reads sequence events from track zero,
including text meta types 1–7. `[`, `]`, `][` and `:` are retained with their
original ticks and event types; tempo events retain microseconds per quarter.
Other tracks' sequence events are counted separately. Unsupported format 2,
SMPTE/zero divisions, bad chunks, unterminated tracks, invalid tempo sizes,
oversized VLQs and missing running status fail explicitly.

Fourteen source files contain bytes after the first track end event, often a
duplicate end event. The pinned converter stops at the first event and advances
by the declared chunk length. The plan follows that boundary and records ignored
byte counts/hashes. It does not reinterpret the trailing bytes as playback.

`mus_encounter_suspicious` has nine literal CC7 values of 128. Pinned `mid2agb`
reads those bytes. They remain explicit in the plan; a renderer that clamps or
masks them must pass a separate compatibility decision/regression first.

MIDI ticks and nominal tempo are **not decoded PCM frame positions**. Source
tick conversion, tempo quantization, mixer timing, channel limits, envelopes and
reverb can affect a render. Every `loop_pcm_frames` remains null and every
`render_verified` remains false. The existing 0.5-second music crossfade is a
remaster presentation policy; it is not source MPlay fade timing. No preview
fadeout may become a loop asset without explicit extraction and seam checks.

## Evidence and reproduction

All 209 MIDI file bytes were fetched from the pin and verified against both
Git blob receipts and the existing catalog's SHA-256 values. The generated plan
also pins all ten converter source/header files and owning production inputs.
No MIDI, AIFF, WAV, OGG or compiled tool payload is added to the public repo.

Regression-first RED: the new tests failed before the plan module existed.
Local GREEN: 13/13 new tests passed. The suite compiles the unmodified pinned
`mid2agb` with g++ and runs **all 209 conversion recipes**, independently checking
voicegroup/volume/priority/reverb headers, nonzero track counts and loop GOTO
counts. Eight-bit controller and duplicate-end handling have synthetic tests.
Malformed MIDI, invalid recipes, source counts/hashes and stale output are gated.
The partial local snapshot's combined discovery ran 31 tests: 30 passed and
one existing full-catalog test could not run because its source header snapshot
was absent. This is not reported as a full-suite pass. The published checkout's
complete R15/Python and CTest suites remain the closure gate.

```
python tools/build_r15_bgm_plan.py --check
python -m unittest discover -s tests -p 'test_r15_bgm_plan.py' -v
```

The R15 workflow checks exact regeneration and runs these tests in both Python
suites. Hosted targeted/full Python and portable CTest results must be recorded
from the published commit before this checkpoint closes.

## Remaining gates and next subphase

This closes source recipe/timeline planning only. Next is
**R15-I2-Closure-2 — pinned source render pilot and decoded loop evidence**.
The pinned poryaaaa CLI can read source MIDI/voicegroups/AIFF without a ROM.
The subsequent four-file actual pilot is recorded in
[R15_BGM_RENDER_PILOT.md](R15_BGM_RENDER_PILOT.md). It must retain
source settings and expose deviations, including CC7=128 and VBlank/mixer timing.
Private render payloads, exact loop frames/seams, transition context, all effects,
host attachment and quality gates remain open. Unreal/UHT/cook and Android/device
claims remain false and belong to their explicit environment/runtime gates.
