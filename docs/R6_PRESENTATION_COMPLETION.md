# R6 Character Presentation — Source and Preparation Acceptance

Status: **pre-real-Unreal implementation and acceptance VERIFIED_COMPLETE on the R6 branch; merge/main gates pending**.
This closes the pre-real-Unreal scope. Actual import, animation/retarget validation,
normalized model output and Android execution remain explicit R18/local asset work.
No playable Unreal or Android result is asserted.

## Coverage and source choices

The active pinned Vanilla+ catalog remains **681 identities: 139 human,
447 Pokémon overworld and 95 special objects**. Every identity has a mandatory
visible fallback. Pokémon model ownership remains R14.

Human coverage is **18 player + 121 NPC identities**:

| NPC decision | Identities |
| --- | ---: |
| Selected exact ORAS source or explicit base variant | 46 |
| Intentional reuse of the same Brendan/May character geometry | 12 |
| Generic source deferred, visible placeholder | 45 |
| Legacy/secondary source deferred, visible placeholder | 13 |
| Verified source gap, visible placeholder | 5 |

Including the 18 player identities, 76 human identities have selected source
geometry and 63 have explicit gaps/deferred sources. These are source choices,
not 76 ready Unreal meshes. All 681 runtime defaults remain visible placeholders
until validated local outputs are supplied.

`npc_presentation_plan.json` supersedes the unresolved decisions in the historical
I3A/I3B inventory; those source inventories and the I3C-1 16-character mapping are
retained unchanged for audit continuity.

The 43 existing private NPC archives were read back at pinned input commit
`75303f4e49de60015d8ffcd28a56b6195d7cb119`. Archive SHA-256, CRC, all 49 DAE member
hashes and the 43 matching provenance records were verified. Public preflight
stores hashes, counts and basenames only. No commercial payload was added.
Joint-name/scene fingerprints are inspection evidence, never retarget approval.

| Multi-model decision | Selected member | Evidence / policy |
| --- | --- | --- |
| Tate | rstr0023_00_fi.dae | Bun without Liza's side hair tails |
| Liza | rstr0024_00_fi.dae | Long side hair tails |
| Gabby / female reporter | rstr0147_00_fi.dae | Female reporter model |
| Ty / cameraman | rstr0148_00_fi.dae | Cap and shoulder video camera |
| Wally | rstr0010_00_fi.dae | Explicit bagless base-geometry choice |
| Youngster | rstr0101_00_fi.dae | Root base member; both Alts excluded |
| Male running Triathlete | rstr0140_00_fi.dae | Red runner with sunglasses |

Textured static source previews were inspected; this was not an Unreal import
or skeleton-pose test. Identity references: [official Tate/Liza concept art](https://archives.bulbagarden.net/wiki/File:Tate_and_Liza_ORAS_concept_art.png),
[Gabby and Ty](https://bulbapedia.bulbagarden.net/wiki/Gabby_and_Ty),
[Triathlete reference](https://bulbapedia.bulbagarden.net/wiki/Triathlete).

The Triathlete package's second member, rstr0142, is a male swimming Triathlete,
not a female runner. The archive note states the cycling model was not exported.
Female runner, both cycling identities, generic male reporter and generic male
swimmer stay unbound. A swimming Triathlete cannot silently fill SWIMMER_M.
TUBER_M_SWIMMING deliberately reuses the Tuber boy's base geometry; its missing
swim animation is not represented as ready. Bike/surf/action rival IDs similarly
share their own character base without inventing equipment or avatar state.

## Animation, materials and preparation

`animation_contract.json` defines idle/walk/run/turn/interact, special player
semantics, exact/family/reference-pose fallback and cosmetic-only timing/notifies.
There are zero verified animation clips and zero verified shared retarget pairs.
The unassigned `special_human` sentinel is not a shared skeleton compatibility
claim. Brendan and May retain their separate player families.

`normalization_contract.json` records centimeter/Z-up/X-forward targets, reviewed
feet origin and yaw, source/bind-pose checks, opaque/masked material policy,
1024px texture targets, ASTC device validation, four material/influence budget
targets and 1/0.5/0.25 LOD ratios. These are preparation targets, not measured
Android performance or generated LODs. Source unit declarations do not calibrate
final world scale. Manifest scale 1 / ground 0 / yaw 0 remain uncalibrated defaults.

`build_r6_normalization_plan.py` produces 47 resumable model jobs covering all
76 selected human identities. NPC jobs pin the chosen member hash and expose
source slot/bone counts, proposed logical material slots and budget-review flags.
Player base-member selection uses the existing private I2B audit before export.
No job claims completed normalization, imported clips or runtime verification.

## Read-only presentation and missing assets

The native-tested `RemasterCharacterPresentationRead.h` reads R4 live object
coordinates, activation and flag visibility without changing save/runtime bytes.
The Unreal adapter joins graphics identity from R4's current object template.
NPC instance ownership is map plus local ID; two NPCs sharing a graphics ID
remain separate. Facing/motion semantics are not exposed by R4 and are not
simulated. Elevation is retained as a collision layer, not invented world height.

`RemasterNpcPresentationWorld` follows those snapshots, removes invisible/stale
instances and discards instances on every map-change event, including same-map
reload. It never decides activation, collision, scripts or flags. Player base
geometry uses the core gender getter; normal is a safe visual fallback and does
not assert an authoritative on-foot/bike/surf state.

`RemasterCharacterVisualComponent` consumes the catalog and a local optional
`RemasterCharacterAssetSet`. It loads only an exact logical model ID when the
manifest's normalized hash matches the binding receipt and the local import is
explicitly validated. Missing catalog, absent binding, unvalidated output or
failed load retain a visible basic shape. Humans use cylinders, Pokémon spheres,
objects cubes; the fallback logical ID is attached as a component tag.

Async requests are cancelled on identity/catalog changes and destruction. Weak
object references and the natively tested generation guard reject stale
completion callbacks. Mesh components have no collision or overlap behavior.
The current skeletal path displays reference pose; no animation blueprint,
root-motion movement, animation-driven script or completion signal is installed.

## Local workflow and cook boundary

From the repository root, with no network or private account dependency:

```sh
python tools/build_r6_character_package.py unreal/Content/Generated/Characters
python tools/build_r6_character_package.py unreal/Content/Generated/Characters --verify
python tools/build_r6_normalization_plan.py generated/r6/normalization-jobs.json
python tools/inspect_r6_character_archive.py local/r6/source/source.zip generated/r6/source-report.json EXPECTED_SHA256
```

Prepare source/exported models under ignored `local/r6`, using the selected jobs.
Record exact DCC/exporter/engine versions, bake reviewed transforms consistently
with skin bind matrices, review weights/materials/LODs, export and hash the actual
normalized bytes. Validate the import in the project-selected UE 5.8.3 environment
before populating normalized hashes and calibrated scale/ground/yaw metadata.
Do not mark a receipt ready based on source hashes or this report alone.

Create a local `RemasterCharacterAssetSet` DataAsset under `/Game/Local/R6`, keyed
by exact `model_id`, with mesh soft reference, output SHA-256 and import-validation
flag. Configure `RemasterCharacterPresentationSettings.AssetSet` locally. Add the
local DataAsset and its referenced meshes to an explicit local cook inclusion
(e.g. PrimaryAssetLabel or `/Game/Local/R6` cook directory), then validate cooked
resolution on device. Public defaults require only engine basic shapes.
The generated catalog directory is staged as UFS by `DefaultGame.ini`.
Both local imported assets and generated character catalog outputs are ignored.

`manifest.sha256` hashes exact staged bytes; `content_sha256` hashes canonical
entries/fallbacks. `--verify` rejects corruption, fractional/bool identities,
duplicate presentation identities and coverage/model mismatches. Run it immediately
before cook. Runtime currently validates structure and hash formats; it does not
recompute the canonical SHA-256. This is a documented build-time integrity boundary,
not runtime tamper protection.

## Verification and remaining boundaries

- 52 R6 Python tests passed locally.
- All 181 Python regression tests passed locally.
- All 20 core C translation units compiled with strict warnings/errors.
- Shared R6 C++ read-only adapter and load-generation test passed.
- 48 existing standalone native C regressions passed through direct GCC builds.
- Unreal source architecture validator passed; generated package is byte deterministic.
- Hosted full CMake/CTest: **62/62 passed**, including the shared R6 adapter,
  Unreal C++ core embedding, R4 and earlier save/script/battle/QoL regressions.
  Local CMake is unavailable; this result is from the hosted CI job.

`r6-character-presentation.yml` runs R6/public metadata tests, all Python regressions,
full portable CMake/CTest including Unreal C++ core embedding, and source guards.
It uploads metadata only and has no timer or self-commit behavior.

Unreal Header Tool/compiler, actual mesh/material/clip import, visual alignment,
shared retarget compatibility, prop completeness, cook validation, Android FPS/memory
and device results remain untested. Missing local assets are accepted through the
explicit fallback contract. No Pokemon asset pipeline is duplicated here.

## Hosted acceptance and final gate evidence

Verified implementation commit: [f9dbf2d20bb6bd38ae921a9aa2225aa969c984b8](https://github.com/illetyus/pokemon-emerald-remastered/commit/f9dbf2d20bb6bd38ae921a9aa2225aa969c984b8).
[CI run 37596293052](https://github.com/illetyus/pokemon-emerald-remastered/actions/runs/37596293052)
completed with **success**, both required jobs passed:

| Gate | Result |
| --- | --- |
| source-and-package | 52 targeted R6 tests, all 181 Python regressions, package/preparation jobs, architecture guards: passed |
| portable-regressions | Full CMake compilation and 62/62 CTest checks: passed |
| Published bytes | All 34 changed blob hashes match locally verified files |
| Branch ancestry | Direct parent is b33f30d; ahead 1 / behind 0 at implementation acceptance |
| Public boundary | No new commercial/archive/model/image/Unreal binary paths |
| Main boundary | Main remains a7ed422da7817c05aa1eac21bdbca6bd6edced5e |

R6-I3/I4/I5/I6, T1, V1 and G1 are closed for source/preparation acceptance.
D1 records this evidence. This documentation-only checkpoint reconciles the
roadmap and execution plan; its exact HEAD CI must also be terminal-success
before any separately authorized merge.

PR creation, merge and main changes remain outside the current authorization.
After the separately authorized R6-M1 merge and R6-M2 main CI gate, the next
roadmap phase is R7-P1. Source preparation acceptance is not real Unreal runtime
acceptance; the external R18 checks above remain mandatory.
