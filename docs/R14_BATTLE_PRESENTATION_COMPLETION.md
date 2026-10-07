# R14 — battle presentation and Pokémon asset preparation

## Scope and checkpoint

Source/preparation implementation is complete; hosted acceptance is pending at
this checkpoint. R14-P1/P2, I1-I8, T1 and local V1 are implemented. G1/D1/F1
remain pending exact published-commit CI and independent blob reconciliation.
This is not a real Unreal build, imported Pokémon pack or Android runtime claim.

The user's explicit R14 start instruction supersedes the previous R9-only
resume boundary. Branch `r14-battle-presentation` is based on verified R9
`4b0da018913867e9f8e83d76c0cb3662501ba3cd`. Main remains
`a7ed422da7817c05aa1eac21bdbca6bd6edced5e`. R6/R7/R9/R14 PR/merge/main gates
remain separate integration work. No R15 work starts under this instruction.

## Source inventory and independent audit

The pinned Vanilla+ source is `70db90c9077aed1272e746fc2537d9f12b95a91c`.
`tools/build_r14_battle_package.py` regenerates the public source-only audit and
the same species/event/trainer catalog used by the native and Unreal consumer.
`--check` requires byte-exact regeneration. No core/vendor files are changed.

| Audited item | Count / disposition |
| --- | --- |
| National Dex species | 386, unique #001–#386 |
| Exact form and normal/shiny model keys | 832, all explicit missing local bindings |
| Modern 3D species models available/imported | 0 / 0 |
| Species with visible primitive fallback | 386 |
| Source trainer picture identities | 93, all missing battle mesh with human primitive fallback |
| Non-NONE R13 event/VFX semantics | 25, explicit primitive pulse and text fallback |
| Required motion semantics | idle, entry, attack, hit, faint; absent clips are explicit |

An independent test reads `sSpeciesToNationalPokedexNum` from raw `pokemon.c`,
resolves species constants and compares all 386 pairs. The internal Emerald
species number is not a National Dex number: Treecko is 277/252, Chimecho is
411/358. Old Unown placeholders, Egg and unknown numbers are explicit unsupported
identities. Source names, Pokédex heights, front/back PNG hashes, constants and
source file hashes are reconciled. Castform's composite source pictures reference
eight physical form PNGs, not an invented single PNG.

Unown has 28 personality-selected forms; extended graphics IDs also retain their
exact form. Castform reads the current core type, without implementing Forecast
or weather decisions in presentation. Deoxys uses Emerald's speed form. Spinda
retains personality metadata but rejects generic 3D bindings until its spot
material host is verified. Transform reads current battle species rather than
original party box species. Shiny selection uses source OT/personality XOR and
threshold 8 without calling RNG.

## Local extraction, normalization and provenance contract

1. Use the user's existing, owned ORAS extraction locally. Do not make 386
   individual downloads a project dependency. No owned species extraction was
   found in this workspace, so the audit records that source as missing.
2. Inspect the model, skeleton, textures and animation files together. Record
   file hashes, source family, exact identity and explicit required special
   channels in a receipt. The receipt is a trusted operator declaration, not
   independent proof of ownership, source format or licensing.
3. A possible local Blender bridge is the upstream
   [n3ds_importer](https://github.com/sxrmss/n3ds_importer). Its README explicitly
   excludes material/visibility animation and export support. Such channels must
   remain recorded and blocked; successful skeletal conversion cannot erase them.
   The tool was inspected, not installed or used on actual game assets here.
4. Export a triangulated, calibrated glTF 2.0 intermediate in metres, Y-up,
   following the [Khronos specification](https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/Specification.adoc).
   Bake mesh and mesh-ancestor transforms for this bounded probe. Use external
   local buffers/PNG textures; unsupported extensions, instancing and morph
   requirements fail visibly. Convert to centimetres, Z-up, forward X and a
   ground pivot during actual engine import; record measured scale and offset.
5. Provide three LODs with at most 20,000 / 10,000 / 5,000 triangles, four material
   slots, 128 bones and 1024-pixel textures. Inspect all five animation semantics;
   a null clip is an explicit fallback. These are preparation budgets, not
   measured Android performance results.
6. Run `python tools/validate_r14_local_assets.py local/r14/bindings.json
   local/r14/audit.json` (join the command onto one line). The binding schema is
   `r14-local-bindings-v1`, with `models` records as described below. Reports are
   `PREIMPORT_VERIFIED` only and always contain `unreal_import_validated: false`.
7. Import privately under `unreal/Content/Local/R14/` or generated Pokémon content,
   which is ignored by git. Configure a local `URemasterPokemonAssetSet` using
   exact keys and hashes only after real import, calibration and skeleton/clip
   checks. Its engine readiness and special-channel flags default to false.

Each binding contains `identity`, `source` and `provenance` records of
`{"path":"relative-local-file","sha256":"64 lowercase hex characters"}`;
`lods` contains three such glTF records. Declare `intermediate_units: "metres"`,
`intermediate_up_axis: "Y"`, `engine_scale`, `engine_ground_offset_cm` and all
five `animation_semantics` entries (clip name or null). The hashed receipt
contains the same `identity`, `source_sha256`, `user_owned_source: true`,
`source_family: "oras"` and `required_special_channels` list. The list must be
explicit even when empty. Nonempty material, visibility, morph, UV or texture
pattern requirements remain blocked until an appropriate host exists. Unknown
form keys and Spinda generic bindings also fail explicitly.

The probe checks actual bounded vertex/index/buffer data, calibrated rest bounds,
LOD counts, skin attribute/joint references, normalized weights, named animation
channels/timelines, external PNG dimensions/CRCs, hashes and local path containment.
It is not a full glTF schema, deforming animation/bind-pose, texture appearance,
Unreal importer or runtime validator. Missing PNGs are explicitly reported.
Synthetic fixtures are original triangles and clips, not commercial payloads.
Reports remain local; no private model paths or extracted payloads enter the
public baseline audit. The runtime consumes trusted local UE assets and readiness
metadata, rather than fetching from the private asset repository.

## Core ownership and sequencing

`RemasterBattleRead.h` is the shared, engine-independent const consumer. It copies
active battlers, source identity/form, HP, current type, status, trainer picture,
weather and outcome. It does not retain a mutable core pointer. Single/double
positions, placeholder height, camera focus and five motion fallbacks are visual
policies. Primitive height is the clamped Pokédex height, not measured mesh scale.

The authoritative host owns the following integration sequence:

1. Assign a positive, monotonically increasing battle epoch and call
   `BeginCoreBattle`. The core host starts R13 and publishes the start batch.
2. After each authoritative operation, call `PublishCoreBatch` with the next
   serial, before clearing events. `FirstEvent` permits a checked suffix when
   the host tracks an append-only batch. The host clears events only after
   successful capture or an explicit visual resync decision.
3. If a 1,024-cue queue rejects a whole batch, retry visual capture or invoke
   `ResyncCoreSnapshot` with a newer serial. Resync copies current core state,
   explicitly counts/logs dropped visual cues and invalidates old callbacks.
   Its drop counter is cumulative for the consumer lifetime. Core resolution
   must never wait for animations or an overflowing presentation queue.
4. The stage completes only the active epoch/token pair. Duplicate, stale,
   out-of-order and cross-session callbacks cannot consume another cue.
5. When R13 has ended, its host owns finalization/save effects and calls
   `EndCoreBattle`. Only this authoritative end releases the UI battle busy
   flag; animation completion and clearing visuals never commit or advance R13.

All 25 raw event kinds and battler/target/move/value/aux payloads are retained.
Substitute damage does not animate a hit on the Pokémon model. Unknown events
receive an explicit unsupported presentation identity. History is bounded at
256. Each event batch carries the final authoritative snapshot of that batch:
this is not a reconstruction of historic HP, form or switch state per event.
R13 may truncate its own saturated 256-event buffer before presentation sees it;
R14 does not claim detection/recovery of already missing core events. A finer
cinematic history would require a separately reviewed authoritative capture
contract. No gameplay logic was changed to create one here.

## Unreal source presentation

The source stage has four battler and two trainer roots, labelled visible
primitives, separate static/skeletal visuals, a camera, floor and one pulse VFX.
The GameMode creates it above the world; it takes the view only for battle
presentation and restores a valid previous view without taking another owner's
camera. Components cannot collide, overlap or affect navigation.

Exact local keys load asynchronously. Slot generation, key checks, weak callbacks,
identity changes, resets and EndPlay cancel stale requests. Missing, invalid or
failed loads preserve a visible fallback. Actual skeletal clips require the same
skeleton; each absent clip retains its motion fallback. Unsupported special
channels remain blocked. No R6 overworld trainer mesh is guessed as a battle
binding; player trainer appearance is explicitly unresolved. Source cue durations
are presentation policy and do not wait for or control gameplay outcomes.

The C++ host entry points are available for R13 integration. This phase does not
claim an already wired, playable Unreal battle input/finalization owner. That
host, UHT compilation, real clip durations/poses, camera readability across four
slots, form/material correctness, engine imports, cook and Android measurements
remain R8/R18/runtime checks.

## Validation and reconciliation

Local targeted acceptance: 24 Python tests and 1,251 native checks passed,
including real single/double R13 battles with immediate, delayed and cleared
visual feeds. Whole battle-state byte comparisons include outcomes and RNG.
Source guards and byte-exact package generation passed. All 236 Python regressions passed. Hosted full CMake/CTest acceptance remains
pending at this checkpoint; CMake is unavailable in the local runtime.

Independent source reconciliation covers all species, exact form keys, physical
sprite references, trainer picture constants and the canonical event enum (the
buffer capacity constant is excluded). Published blobs, dependency parent,
unchanged core/vendor and terminal exact-head CI must be checked before marking
G1/D1/F1 verified. R14-M1/M2 remain pending; next preparation phase is R15 Audio.
