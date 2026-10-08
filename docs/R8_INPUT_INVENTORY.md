# R8-P1 — Common action and owner inventory

State: **VERIFIED_COMPLETE** once this document is versioned.
Audit base: `b8d789e697ee06c466c5546dfbebfb546b4e89e2`.
Branch: `r8-input-infrastructure`, created from that verified main.
Next named subphase: **R8-P2 — Platform/lifecycle contract**.
This checkpoint changes documentation only; no input/gameplay behavior changes.

## Authority and entry point

Production gameplay authority is `illetyus/pokezumrut-vanillaplus` at
`70db90c9077aed1272e746fc2537d9f12b95a91c`.
Pinned `src/field_control_avatar.c` distinguishes new and held keys, derives
direction in Up/Down/Left/Right priority and passes field actions through
`ProcessPlayerFieldInput`. Pinned `src/menu.c` distinguishes `JOY_NEW`
confirmation/cancellation from direction repeat. These are source evidence,
not permission to transplant GBA key bitfields or gameplay scheduling into UE.

Current production config selects `R0GameMode`, whose constructor selects
`ARemasterPlayerController` and `ARemasterOverworldPawn`.
`AR0PlayerController` and `RemasterCoreSubsystem` retain the R0 demo boundary;
their touch quadrants and reset-on-R binding do not implement production input.

Audited project files at the immutable base:

- `core/include/remaster/core.h`, `emerald_movement.h`,
  `emerald_overworld.h`, `emerald_qol.h`, `emerald_script_host.h`,
  `emerald_script_runtime.h`, `emerald_battle.h`;
- `unreal/Source/PokemonEmeraldRemastered/RemasterPlayerController.{h,cpp}`,
  `RemasterInputConfig.h`, `R0PlayerController.{h,cpp}`, `R0GameMode.cpp`;
- `RemasterWorldGameplaySubsystem.{h,cpp}`, `RemasterUISubsystem.{h,cpp}`,
  `RemasterUiModel.{h,cpp}`, `RemasterUiWidget.cpp`;
- Unreal config, R9 completion ledger, `tests/test_r9_ui.py`,
  `tests/r9_ui_model_test.cpp`, `tools/validate_unreal_source.py`,
  `CMakeLists.txt` and canonical architecture/roadmap/phase plan.

## Existing semantic actions and mappings

R9 exposes the same ten names in its portable model and UE enum. Physical
bindings below describe current bytes, not the future frozen R8 mapping.

| Common action | Keyboard fallback | Gamepad fallback | Existing consumer |
| --- | --- | --- | --- |
| Up | Up / W | D-pad Up | R9 first, otherwise authoritative north step |
| Down | Down / S | D-pad Down | R9 first, otherwise authoritative south step |
| Left | Left / A | D-pad Left | R9 first, otherwise authoritative west step |
| Right | Right / D | D-pad Right | R9 first, otherwise authoritative east step |
| Confirm / field Interact | Enter / Space | Face Bottom | R9 confirm; unconsumed field interaction is BP entry |
| Cancel | Escape | Face Right | R9 cancel; fallback opens menu if unconsumed |
| Menu | Tab | Special Right | R9 menu |
| Map | M | none | R9 region/map page |
| Quest | Q | none | R9 objective page |
| QuickItem | R | Face Top | R9 quick-item page; field effect is host entry |

The optional InputConfig has a mapping context plus Move (2D), Interact,
Cancel, Menu, Map, Quest and QuickItem soft InputAction references.
Each available Enhanced action suppresses that action's native fallback and
binds only Started. The mapping asset itself is not a committed cooked asset.
Move takes the dominant axis, ties favor Y, and has no dead-zone test.
No production press/release/cancel aggregation, touch ownership or reconnect
identity contract exists yet.

Enhanced and fallback Cancel differ outside UI: the former emits BP_OnCancel,
the latter opens Menu. Map/Quest have no native gamepad equivalents.
These are I3/I4 closure requirements, not accepted equivalent mappings.

## Core command and owner boundaries

| Surface | Existing authoritative operation | Current production host disposition |
| --- | --- | --- |
| World direction | R4 `remaster_emerald_overworld_step_action`, continuation and transition/event/encounter processing | Gameplay subsystem StepPlayer owns the save/core mutation; controller applies returned presentation snapshot |
| Field interaction | R2 runtime dispatch_object/dispatch_background and pending requests | Controller exposes BP_OnInteract; the audited controller does not own or dispatch a script VM |
| Dialogue/choice | R2 pending request + sequence-checked runtime_complete | R9 PresentCoreDialogue consumes an externally owned runtime and resolved resources; completion is explicit |
| Menu/party/bag/settings/save | R9 model stages revision/fingerprint-checked intent and delegates to accepted domain/platform surfaces | UI owns presentation and permitted domain command adapters, not movement/VM execution |
| Battle selection | R13 Move/Switch/Item/Run action structs, resolve_turn/replacement/finalization APIs | R14 consumes committed const battle state; an actual battle request owner is not asserted by this inventory |
| Nickname/field-item entry | R9 OnNicknameEntry / OnFieldItemEntry with revision | Editor/effect host attachment remains an explicit existing integration obligation |
| Audio | R15 const committed request consumer and source waits | Input must not infer completion from playback or publish duplicate requests |

R0 RemasterInput's five demo commands are not the authoritative Emerald command
schema. R8 routes semantic intent to these existing owners; it must not introduce
a second battle, story, script, bag, movement or save truth. An unattached owner
must return an explicit blocked/unsupported result, never silently consume a
command as successful or fall through into world movement.

## Existing focus and timing behavior

StepDirection calls RouteUI before StepPlayer. R9 BlocksWorldInput combines
modal state, save/map usability and script/battle/I/O busy state.
NotifyCoreStep latches script/encounter busy; the actual host must clear it
after authoritative work. Dialogue lifetime stays with its runtime owner.

R9 vertical menu repeat uses the portable Repeat structure: first repeat after
24 presentation ticks, then every 3 ticks at 60 Hz. Confirm and world movement
are never synthesized by that repeater. However, the UE presentation timer polls
physical Up/W/D-pad and Down/S/D-pad keys separately; touch/Enhanced held state
cannot currently supply the same repeat path. MenuRepeat/FastText preferences
are stored in GameUserSettings outside Emerald saves.

R8 must preserve that presentation-only cadence and replace device-specific
polling with common held state. It must not replay elapsed background time into
world steps, RNG, script waits or battle actions.

## Gaps mapped to named subphases

| Gap | Owner |
| --- | --- |
| No bounded platform-neutral press/release/cancel/action-source contract or finite/dead-zone axis normalization | P2 / I1 |
| Production controller has no touch layout, finger capture, moved/released handling or cancel-on-exit path | I2 |
| Started-only Enhanced bindings, no equivalent native Map/Quest gamepad mappings, Cancel behavior differs | I3 |
| Shared UI/world/battle/script focus arbitration, explicit unsupported-owner response and no stale event across focus change | I4 |
| No production input lifecycle reset/delegate teardown/controller disconnect/reconnect state fencing | I5 |
| Device-key polling in R9 vertical repeat; touch/Enhanced common hold and preference reset not connected | I6 |
| Compiled shared router, mapping equivalence, multi-source holds, stale/focus/lifecycle/repeat regressions | T1 / V1 |

P2 must choose and version exact source/device identity, duplicate-event policy,
focus precedence, reset epochs, opposite-direction/axis policy, repeat boundary
and touch coordinate/safe-area rules before implementation.
Actual Unreal/UHT compilation, imported mapping assets, touch hit testing,
Android lifecycle/device latency and physical controller behavior remain R18/
real-runtime checks. No source test substitutes for those checks.

## Verified integration baseline

Each phase below merged through a PR only after its final source gate and exact
PR checks succeeded. All relevant workflows on each resulting main HEAD reached
terminal success, including the dynamic CodeQL scan, before the next phase.

| Phase | PR | Main merge | Main phase workflow | Actual targeted Python / full Python / CTest | All main workflows |
| --- | --- | --- | --- | --- | --- |
| R6 | #17 | `a50209eb82d7e67ae332da1584d5d0ac5b039db0` | 37805647753 | 52 / 181 / 62 PASS | 10/10 success |
| R7 | #18 | `a87709f1cb76d92b4394d392188fdfe58df2d2fd` | 37807144475 | 21 / 202 / 63 PASS | 11/11 success |
| R9 | #19 | `0512dc9d9c3d0a172c454c37d388e235760292b5` | 37808838838 | 10 / 212 / 65 PASS | 12/12 success |
| R14 | #20 | `64328703f0554c943463453b1a5f6167f7346693` | 37810837127 | 24 / 236 / 66 PASS | 13/13 success |
| R15 | #21 | `b8d789e697ee06c466c5546dfbebfb546b4e89e2` | 37812397707 | 74 / 310 / 67 PASS, zero Python skips | 14/14 success |

R7/R9/R14/R15 ancestry reconciliation commits changed zero files and retained
each accepted tree exactly; no force update or vendor/payload change occurred.
Older completion documents' M1/M2-pending statements describe their historical
publication snapshots. This live integration ledger supersedes those resume
statements without promoting any deferred asset/runtime obligation.

## P1 verification boundary

Live main/branch refs, recent history, compare, open PRs, exact main CI and the
canonical/owning source/test docs were reread. No concurrent drift was present.
This is a documentation checkpoint. Existing implementation test results above
are baseline evidence; no new execution or engine/device test is claimed.
Post-commit exact changed files and workflow-trigger disposition must be checked.

