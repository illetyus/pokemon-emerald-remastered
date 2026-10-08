# R8-P2 — Platform-neutral input and lifecycle contract

State: **VERIFIED_COMPLETE** once this contract and its schema are versioned.
Audit base: `261cef110a06fdedd9ec57dc66ffb026330d12e3`.
Owning machine-readable artifact: [input_contract.json](../data/r8/input_contract.json).
P1 evidence: [R8_INPUT_INVENTORY.md](R8_INPUT_INVENTORY.md).
Next: **R8-I1 — Common action router**, regression before implementation.
This checkpoint freezes policy only; no implementation behavior changes.

## Authority and policy boundary

The pinned production source is
`illetyus/pokezumrut-vanillaplus@70db90c9077aed1272e746fc2537d9f12b95a91c`.
Field input distinguishes new versus held keys and prioritizes Up, Down, Left,
Right. Menu confirmation/cancellation uses new keys; menu navigation can repeat.
Accepted R9's portable vertical repeater owns the 24-tick/3-tick presentation
policy. Gameplay cadence, collision, movement mode, battle legality, RNG, VM
execution, source waits and save state remain existing core/host authority.

Modern source identity, analog dead zone, normalized touch layout, duplicate
suppression and lifecycle epochs are explicit platform policies. They are not
claimed to be native GBA hardware behavior. Dominant-axis quantization keeps
the current controller's Y tie break; priority between concurrently held
digital directions uses the pinned field source order.

## Action and event schema

Ten common actions: Up, Down, Left, Right, Confirm, Cancel, Menu, Map, Quest,
QuickItem. Confirm is field Interact only in world focus; it does not bypass
R9 confirmation/choice revisions in modal/dialogue focus.
No action directly sets flags, vars, HP, items, battle outcomes or saved state.

An event identifies source (Keyboard/Gamepad/Touch/Enhanced), device and
physical control, plus action, phase and host epoch. Pressed, Released and
Canceled are separate. Physical control identity must distinguish aliases
(e.g. Up and W) and touch fingers; each adapter retains its press epoch through
release. The UE host supplies current focus/context; device events do not choose
or persist gameplay owners.

The shared router has 64 bounded held records. The same physical press is
idempotent; a second control holding the same semantic action does not dispatch
another command. Release/cancel affects only its matching control/action.
Unmatched release is ignored; action mismatch is rejected without mutation.
Capacity overflow does not evict another held record.
Malformed enums and nonmatching/zero epochs fail explicitly before mutation.

World directions dispatch only on a new selected direction press. Releasing a
higher-priority direction does not synthesize movement for another held key.
This stage preserves explicit press commands; it does not create an autonomous
world repeat scheduler. Confirmation, menu entry and battle commit never repeat.

## Focus and owner routing

Focus/context is refreshed before delivery. Precedence is suspension, I/O busy,
attached pending dialogue, battle, other script busy, modal UI, usable world,
then available recovery UI or blocked. Recovery UI preserves R9 Menu/load access
when no usable save/map exists; it must never enable world movement. UI presentation itself cannot clear core host busy lifetimes.

| Focus | Direction / Confirm / Cancel | Menu / Map / Quest / QuickItem |
| --- | --- | --- |
| World | Direction → R4 StepPlayer; Confirm → explicit field-interaction owner; Cancel → R9 | R9 |
| Modal UI / Dialogue / Recovery UI | R9 shared action path; choice/row revision checks remain authoritative | R9 model decides; safe Menu/load recovery remains available |
| Battle | Attached battle-input owner, with current core request/selection revision | Explicit blocked |
| Suspended / I/O / non-dialogue script / no attached owner | Explicit blocked | Explicit blocked |

A missing field/battle owner yields explicit unsupported, never success and
never a fallback world command. A callback/Blueprint entry is not proof that
a script VM or playable battle host is attached.
R8-I4 must version the entry/response and busy/reset boundaries. Existing
R9/R14/R15 outstanding request-owner and resource-attachment obligations stay
visible for the actual host/runtime stage; this contract does not close them.

Changing focus owner or host generation resets common holds, repeat and touch
captures, and advances a nonzero uint64 epoch. Stale queued press/release is
rejected. Epoch exhaustion fails closed; it must not wrap and accept old events.
The new owner's context must not receive held commands from the previous one.

## Mappings and analog input

Keyboard: arrows/WASD, Enter/Space confirmation, Escape cancel, Tab menu,
M map, Q quest, R quick item. Gamepad: D-pad, FaceBottom confirm, FaceRight
cancel, SpecialRight menu, LeftShoulder map, RightShoulder quest, FaceTop quick.
Both Cancel paths use the same semantic route; no fallback-only menu shortcut.

Optional Enhanced Input is an adapter to the same schema. An available action
suppresses only its equivalent native fallback; an absent/invalid action leaves
the fallback. Asset-backed mapping readiness must remain explicit.
Started/changed/released/canceled axis and button events must not duplicate native
bindings. Closure-1 makes equivalence physical-key-specific: only covered keys
are suppressed; uncovered aliases and gamepad/keyboard controls remain native.
Reserved keys assigned to another action, positional Touch keys and incomplete
left-stick axis assets are rejected atomically. Native paired stick polling is
replaced only by complete mapped stick coverage. Cooked modifiers/triggers still
require actual engine/runtime validation. No separate Enhanced gameplay handler is allowed.

Analog coordinates must be finite and within [-1,1]. Magnitudes below or equal
to 0.25 are neutral. Above the dead zone, the largest magnitude selects one
direction; ties select Y. X positive → Right, Y positive → Up.
Neutral/changed direction releases the prior axis control before another press.
Malformed axes do not dispatch or leave a new held direction.

## Touch capture and presentation preferences

The schema contains ten default nonoverlapping half-open rectangles normalized
to the usable viewport after safe insets. Finger IDs 0–9 are bounded captures.
Only a press in a control captures that finger; empty-space presses do nothing.
Moves outside the captured rectangle cancel it, without dragging into a new
action. Release/cancel belongs to the captured epoch and control.

Touch input must share focus and held state, including R9 repeat. Row activation
retains explicit row/revision intent, not a fabricated confirm for an unrelated
selection. A handled UI touch must not dispatch a second controller/world action.
Safe area, DPI, pointer propagation and actual hit regions require real UE/device
verification. Source geometry alone is not that evidence.

Layout/visibility/scale preferences are presentation settings outside Emerald
saves. Invalid rectangles/insets/scales fail validation or retain validated
defaults; no persistent gameplay metadata/layout is introduced.

## Lifecycle and repeat

Background, inactive/window focus and paused are independent suspension reasons.
Any reason resets input and blocks new delivery. Clearing one reason cannot
resume while another remains. Resume flushes platform pressed state and requires
new physical presses; elapsed time is never replayed into gameplay.
Disconnect conservatively resets all input; reconnect never restores old holds.
Teardown removes owned delegates/mapping context and clears captures.

R9 remains the only vertical UI repeat owner: 60 Hz presentation ticks,
first repeat at tick 24 and every 3 ticks thereafter for held Up/Down.
Common held state replaces physical-key polling. Turning MenuRepeat off or
changing focus/lifecycle clears the countdown. Catch-up is bounded to presentation.
No world movement, Confirm, battle submission or VM completion is synthesized.
FastText remains existing R9 text presentation; R8 creates no second setting.

## Implementation and acceptance plan

I1 adds the compiled portable router/event/focus/axis boundary; I2 touch source
geometry/capture/wiring; I3 keyboard/gamepad/Enhanced adapters; I4 owner routing;
I5 lifecycle/delegate/reset integration; I6 common-held R9 timing/preferences.
Each implementation checkpoint pins meaningful regression/RED first and keeps
other subphases explicit. T1/V1/G1 then cover all schema acceptance requirements.

The real UE/UHT build, cooked mappings, usable touchscreen/controller behavior,
Android callbacks, latency and device matrix remain R18/runtime obligations.
No source test or metadata approval is a substitute. The authorized stop remains
REAL UNREAL RUNTIME VALIDATION, or EXTERNAL_ENV_REQUIRED if the R18 environment
is unavailable. Physical Android smoke, BrowserStack, final matrix and R21/R22
do not start automatically.

