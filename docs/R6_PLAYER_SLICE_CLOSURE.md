# R6-I2 Brendan / May Slice Closure

Status: **R6-I2 complete**

Pinned public branch at closure:

`0444ccc9d369876787dfd090c2312e0d5da4a214`

## Scope completed

R6-I2 established the player-character presentation foundation for Brendan and
May without adding proprietary model payloads to the public repository.

Completed slices:

- R6-I2A: verified ORAS Brendan/May source archives stored in the private asset
  vault with provenance;
- R6-I2B: structural model/rig/texture/accessory audit;
- R6-I2C: all 18 Brendan/May graphics identities classified into stable
  presentation states;
- R6-I2D: all 18 identities bound into the deterministic public R6 manifest;
- R6-I2E: closure verification.

## Private source verification

Private vault:

`illetyus/pokemon-emerald-remastered-assets`

Visibility was re-verified as `private` at closure.

Stored source archives were read back and SHA-256 recomputed:

- Brendan ORAS overworld archive:
  `fd6afb77420c622e37e405a44a981aa1c2563c69b2fc3fa693900a0b4637a2f6`
- May ORAS overworld archive:
  `a6a53b88e54b4961e936a48c29084d9dbaa1e51685a745a4b7a70bda0e0cd756`

Both hashes exactly match the public manifest bindings.

## Public/private boundary verification

The R6 branch diff from `main` contains nine changed files and no newly added
FBX, GLB, GLTF, DAE, SMD, Blender, Unreal asset, ROM, archive, or equivalent
proprietary binary path.

The public repository contains logical asset IDs, source hashes, provenance IDs,
state policy, resolver code, tests, and documentation only.

The asset-vault repository name appears only in the architecture contract as a
documented personal vault. Public runtime/build code does not depend on that
repository URL.

The `/tmp/brendan.fbx` string in the manifest test is deliberately invalid
input used to prove that machine-specific asset paths are rejected.

## Manifest closure

All 18 Brendan/May source graphics identities are represented.

Persistent source avatar states:

- normal;
- Mach Bike;
- Acro Bike;
- surfing;
- underwater.

Transient source avatar actions:

- field move;
- fishing;
- watering.

Legacy special identity:

- decorating.

Decorating remains a presentation identity and is not promoted to a gameplay
avatar state.

All player records retain `fallback.human`.

No record claims a normalized Unreal output yet:
`normalized_sha256` remains empty for all 18 records.

No record claims an animation clip has been imported merely because an
animation semantic ID exists.

## Regression verification

Targeted suites:

```text
tests.test_r6_character_manifest
tests.test_r6_player_presentation_states
```

Result:

```text
14 tests
14 passed
0 failed
```

The tests verify:

- the pinned Vanilla+ 681-identity catalog;
- deterministic manifest generation;
- explicit fallback coverage;
- machine-path rejection;
- all 18 Brendan/May identity mappings;
- stable Brendan/May logical model IDs;
- separate player skeleton families;
- source SHA-256 binding;
- decorating-state boundary;
- missing-animation honesty.

## Known deferred work

R6-I2 does not claim:

- Unreal Engine 5.8.3 runtime import verification;
- normalized Unreal-ready model outputs;
- final player animation clips;
- bike/rod/watering/surf-mount assets;
- authoritative portable gameplay exposure of bike/surf/action state.

Those remain later R6 implementation/integration work. R6 presentation must not
infer gameplay state from meshes, animations, map tiles, or movement speed.

## Exit decision

**R6-I2 is closed.**

The next implementation slice is the NPC/trainer character presentation path,
starting with source/coverage inventory and model-family mapping while
preserving the same public/private asset boundary.
