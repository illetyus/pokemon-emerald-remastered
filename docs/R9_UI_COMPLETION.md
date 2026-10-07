# R9 UI / HUD / Menu — Source and Preparation Acceptance

Status: **IMPLEMENTED; final regression and publication reconciliation pending**.
This is the pre-real-Unreal source/preparation scope used for R6 and R7. The
shared UI model is compiled natively; the Unreal subsystem, controller and UMG
widget are source checked. This does not certify a UE/UHT build or device run.

## Screen and authority inventory

| Screen / entry | Data authority | Supported source behavior |
| --- | --- | --- |
| HUD / menu | Core overworld snapshot and R10 active quest | Money, party count, objective, abstract input, bounded back stack |
| Map / quest | R10 quest API, current R3 map context | Source region name, description and exact marker rectangle on the 28 × 15 Hoenn grid; no objective writes |
| Party / summary | R11 Pokémon APIs and R16 information helpers | Checksum-aware party rows, saved stats/HP/moves/PP, six IVs/EVs, total EV and Hidden Power type |
| Bag / item | R11 item/slot APIs and R16 bag commands | Five pockets, quantities, source names/prices, four native sort modes, auto-sort, quick-item registration/removal |
| Quick item / Repel | R16 metadata, authoritative `bRepelWoreOff` event | Native registered-item list and available Repel choices; field-use host entry, never invented effects |
| Relearner | R16 eligibility, candidates and learn API | Native candidate list and replacement slots; Heart Scale consumed only on native success |
| Nickname | R16 OT-name/ID and Egg eligibility | Eligible naming-screen entry hook; actual naming host/string editor remains integration work |
| Settings / RTC | Local UI preferences; R1 clock/correction | Fast text/menu repeat options, bounded local time draft, explicit apply through existing R1 wrapper |
| Save / load | R17 platform wrappers | Confirmation, busy state, actual success/failure, missing/corrupt/unsupported/I/O messages, world reload on successful load |
| Dialogue | Pending native script request and host-resolved resources | Matching type/sequence/program/PC, declared choice values, UTF-8 reveal, explicit completion; no VM run |

Display names are generated directly from pinned Vanilla+ species/move/item and
region tables. Nickname decoding selects the source Turkish/Latin or Japanese
byte page by language; shared byte `01` is **Ğ** or **あ**, not an ASCII character.
Unknown bytes display a replacement glyph. Missing display IDs retain `#id`.
The production source pin is unchanged:
`70db90c9077aed1272e746fc2537d9f12b95a91c`.

The native model owns only screen/focus/draft/presentation state. It reads core
save bytes; metadata getters that initialize their input run on isolated copies.
Commands require a matching frame revision and fingerprint of all persistent
domains/provenance/status. Sorting, registration and relearning stage a save
copy, call the existing native API and commit only on success. Rendering,
navigation, failed/stale actions and RTC drafts preserve authoritative save bytes.
No UI stat formula, quest progression, item effect, encounter RNG or healing rule
has been introduced. Core and vendored source remain unchanged by R9.

## Input and presentation

`ERemasterUiAction` and `ActivateRow(index, revision)` are the shared keyboard,
gamepad and pointer/touch routes. Native buttons retain the revision they were
created from; late clicks fail closed. World-direction input is consumed by the
UI before `StepPlayer`. Modal screens, unusable save/map, script/battle host busy
state and synchronous save I/O prevent world movement. Closing a dialogue cannot
discard a pending script request. Confirm on an empty modal frame stays consumed.

| Fallback input | Route |
| --- | --- |
| Arrows / WASD / gamepad D-pad | World step or modal focus; Bag left/right changes pocket |
| Enter / Space / gamepad A | Modal confirm or authoritative interaction host |
| Escape / gamepad B | Back; from HUD opens menu |
| Tab / gamepad Start | Menu |
| M / Q / R or gamepad top face | Map / quest / quick item |
| Pointer / touch | Revision-checked native row command; Back sends abstract cancel |

Enhanced Input binds supplied actions. Each missing action receives only its own
fallback; a partial configuration no longer suppresses all remaining controls.
A configured MappingContext must actually map the supplied actions. The R8
physical input/device layer is not replaced here. UMG buttons are nonfocusable;
keyboard/gamepad use one native route while pointer/touch targets remain usable.
Safe area, scrollable content, selection scrolling and 64 Slate-unit rows are
provided without imported widget assets. Hoenn artwork and real DPI/touch results
remain runtime checks; the map is a source grid/marker shell.

Presentation uses a bounded 60 Hz local clock, at most four printable UTF-8
glyphs per fast-text tick (one in normal mode), and 24/3 menu-only held repeat.
Core views poll at 4 Hz outside text reveal/navigation updates. Timers never step
the world or complete a script/audio/movement/fanfare/delay request. First confirm
reveals remaining text; only a later explicit confirm completes the native request.
UI preferences use `GGameUserSettingsIni`, separate from legacy save bytes.

R16 ledger decisions remain unchanged: its `REJECTED` presentation rows exclude
those changes from the portable gameplay core. R9 provides QOL-028/029 menu,
QOL-032 in-game RTC, QOL-042 quest-menu surfaces, accepted summary/relearner/
nickname eligibility surfaces, and the QOL-012 Repel event/dispatch entry.
Repel expiration during script/battle waits for the host to report safe idle.
QOL-030 field-move animation and QOL-031 Pokémon Center placement/flash/nurse
timings require their R6/R7/R15 presentation hosts; R9 retains the dialogue hooks
and asynchronous barriers instead of treating these effects as completed gameplay.
Their actual animation/audio timing is not claimed by this infrastructure checkpoint.

## Host integration contract

The existing world/controller had no native Unreal script-runtime owner, resolved
text/choice catalog, naming editor or field-item effect dispatcher. R9 supplies
typed entry points without creating substitute gameplay behavior:

1. The script/battle owner calls `SetHostBusy` for the full authoritative lifetime.
   `NotifyCoreStep` fences source script/encounter events before Blueprint dispatch.
2. For a MESSAGE/CHOICE pending on the same save, the owner resolves source
   resource IDs and dynamic substitutions, then calls `PresentCoreDialogue` with
   resolved text and choice labels/native values. Unresolved resources are not
   fabricated or automatically acknowledged. Choice cancel is deliberately not
   synthesized without source cancellation policy.
3. UI completion calls `remaster_emerald_script_runtime_complete` exactly once
   for that pending request. The owner alone resumes the VM and handles all other
   request types, including fanfare/delay/movement. It detaches before destroying
   or restarting its runtime and reports idle only after the authoritative work ends.
4. `OnNicknameEntry` / `OnFieldItemEntry` are synchronous host-entry events. Their
   consumer revalidates ownership, item availability and the native effect/field
   policy, establishes busy lifetime and uses its existing domain implementation.
   Missing handlers show an unavailable message, never a fake successful rename,
   heal or Repel application.

R17 remains responsible for serialization and persistence. Load uses the existing
explicit Vanilla+ format. A failed load leaves the wrapper's output unusable;
the UI reports that result and blocks world input rather than asserting preserved
active gameplay. A successful load invalidates old UI requests, rebuilds current
map state and resynchronizes the pawn. No optimistic save-success percentage is used.

## Verification and gates

Local evidence: **10 R9 Python source/contract checks**, **215 native UI checks**,
and **10 required-screen native fixtures** passed. Shared C++ compiles with GCC
C++17 and `-Wall -Wextra -Wpedantic -Werror`. Tests compare whole save bytes across
render/navigation/errors, corrupt/checksum-invalid party data, stale revisions and
external core updates, sort/quick-item/relearner outcomes, RTC draft isolation,
real VM sequence/choice handling, UTF-8 glyph pacing and audio-barrier exclusion.
The portable fixture oracle is separate JSON plus a Python consumer of actual
native frames, rather than independently recreated UI logic.

Full Python, hosted CMake/CTest and exact-head GitHub acceptance are recorded
after terminal completion. Unreal source architecture guards and deterministic
display catalog verification passed locally. Real UE 5.8/UHT, rendered screens,
host resource/editor/effect integration, imported map art, input focus/DPI and
Android device behavior remain R18/R19 and relevant host-phase validation.

The user's direct “finish the next phase” instruction authorizes R9 source
preparation before the earlier merge/main gates. R9 is based on verified R7 head
`32850179f41be4c12e2921d956fa52f7f0f0536b`, on dependent branch
`r9-ui-presentation`. Main remains `a7ed422da7817c05aa1eac21bdbca6bd6edced5e`.
R9-M1/M2 remain separate integration gates; R6/R7/R9 must be integrated in order.
Next preparation phase is **R14**; it is not started under this R9-only request.

## Reproduction

```sh
python tools/build_r9_ui_catalog.py --check
python -m unittest discover -s tests -p 'test_r9*.py' -v
python -m unittest discover -s tests -p 'test_*.py' -v
python tools/validate_unreal_source.py
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python tools/r9_ui_fixture_matrix.py --probe build/r9_ui_model_test
```
