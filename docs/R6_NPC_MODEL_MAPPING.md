# R6 NPC Model Mapping

Status: **R6-I3C-1 complete — single-model named-source bindings only**

## Scope and evidence

The existing I3A exact-counterpart inventory and I3B acquisition audit select
16 Unique NPCs (Overworld) packages containing exactly one DAE each. This slice
binds those Emerald graphics identities to their corresponding logical ORAS
models. It does not infer identities from model-number order.

Inputs were read at public commit `ca4fb0d33f7979efc5a08358a4b3c07f3e996991`
and private source commit `75303f4e49de60015d8ffcd28a56b6195d7cb119`.
All 16 existing private source archives were read again: archive SHA-256 and
ZIP integrity match I3B, with zero mismatches. Each archive has the one DAE
recorded by acquisition. Each selected DAE has its own SHA-256 in the mapping.
The 16 existing individual provenance records were also checked against the
index, package hash and DAE selection. No new upstream assets were acquired.

`data/r6/npc_model_mapping.json` records submodel selection and readiness.
`data/r6/character_presentation_overrides.json` contains only fields accepted
by the existing manifest schema. Runtime continues to consume that schema;
this sidecar is audit metadata, not a new runtime parser or asset loader.

## Selected models

| Character | Graphics ID | ORAS asset ID | Selected source file |
| --- | --- | --- | --- |
| Professor Birch | 64 | 296314 | rstr0009_00_fi.dae |
| Sidney | 121 | 296309 | rstr0013_00_fi.dae |
| Phoebe | 122 | 296308 | rstr0014_00_fi.dae |
| Glacia | 123 | 296331 | rstr0015_00_fi.dae |
| Drake | 124 | 296323 | rstr0016_00_fi.dae |
| Roxanne | 125 | 296327 | rstr0017_00_fi.dae |
| Brawly | 126 | 296311 | rstr0018_00_fi.dae |
| Wattson | 127 | 296330 | rstr0019_00_fi.dae |
| Flannery | 128 | 296297 | rstr0020_00_fi.dae |
| Norman | 129 | 296313 | rstr0053_00_fi.dae |
| Winona | 130 | 296310 | rstr0022_00_fi.dae |
| Wallace | 133 | 296329 | rstr0025_00_fi.dae |
| Steven | 134 | 296334 | rstr0012_00_fi.dae |
| Archie | 195 | 296332 | rstr0031_00_fi.dae |
| Maxie | 196 | 296333 | rstr0026_00_fi.dae |
| Player's Mother | 215 | 296685 | rstr0163_00_fi.dae |

## Provenance aliases

The existing NPC provenance documents have an asset ID and source hash but
no `provenance_id` field. This slice introduces stable logical aliases:

`provenance.r6.npc.oras.models_resource.<asset_id>`

Each alias refers to the verified existing provenance record for that exact
source asset ID. It does not assert that this alias was already present in
the private document. The private records are unchanged. Public metadata
contains no private archive/provenance path or machine path.

## Readiness and fallback

- Identity/source selection: verified exact counterpart, single source model.
- Native skeleton: skin controller presence recorded; topology and rest pose
  compatibility remain unverified. Native skeleton IDs are logical labels.
- Skeleton family: `special_human` retains the existing default as an
  **unassigned sentinel**, not a verified shared rig. No adult/teen/child rig
  assignment or family-compatible reuse is authorized by this slice.
- Normalization: pending; `normalized_sha256` remains empty.
- Real animation clips: missing; `animation_set_id = fallback.human` makes no
  claim that a walk, idle or other ORAS clip is ready.
- Scale, ground and yaw: unchanged defaults 1.0 / 0.0 / 0.0, uncalibrated.
- Materials: unchanged empty binding; no normalized material readiness claim.
- Unreal import/runtime: untested. These source bindings do not load meshes.
- Explicit fallback: `fallback.human`, the existing project placeholder policy.
  Visible runtime fallback behavior is still a later R6-I6/R18 validation.

Named characters never substitute for another named character. Ambiguous
Liza/Tate, Interviewers, Wally, Youngster and Triathlete packages remain
unbound. I3B rejects male swimmer and generic male reporter sources; both
remain on the existing placeholder. No Pokémon pipeline is added in R6.

## Validation

Run from the repository root:

```sh
python -m unittest discover -s tests -p 'test_r6_*.py' -v
```

Result on this slice: **37 tests passed, 0 failed** (27 existing + 10 mapping
regressions). Tests cover source/identity agreement, manifest consumption,
unique named models, unknown and duplicate identity rejection, preserved
source gaps, safe public metadata, readiness boundaries and total coverage.
Public tests use public metadata only; they require no private connection.

The manifest CLI was run twice with identical inputs; output bytes match.
All 18 existing player overrides retain their original field values.
The full identity set remains **681 = 139 human + 447 Pokémon overworld +
95 special objects**, IDs 0..680 without gaps.

Generated manifest file SHA-256: `744f7c9d40c79e5551401e1171c40badada330bf833b277cae801e41c5c26c49`.
Generated manifest content SHA-256: `ce8810830e19e17549a9fda6a7d06b89fff6351392af52b9aef7cee34236df2b`.

These are local metadata/manifest checks, not a fresh hosted CI, engine build
or Android result. Generated output is not committed.

## Next boundary

Stop after this slice and report the commit. Next is R6-I3C-2, starting with
one multi-model package (Liza & Tate): visual/source evidence must identify
each submodel before any binding is made. Continue only on user instruction.
PR, merge, main changes and scheduled automation are outside this slice.
