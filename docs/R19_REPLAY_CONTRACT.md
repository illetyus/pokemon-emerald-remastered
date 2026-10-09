# R19-P2 — Portable production replay/hash contract

State: contract frozen for versioned publication; no implementation behavior changes.
Machine-readable authority: data/r19/replay_contract.json, schema version 1.
Prerequisite: R19-P1 at 915fee9253f93241b060882467b4742bd81d63d8.
Production source: illetyus/pokezumrut-vanillaplus@
70db90c9077aed1272e746fc2537d9f12b95a91c.

## Replay and fixture identity

A version-1 matrix identifies source pin, fixture recipe/data identities, unique
case IDs, initial setup, ordered typed commands, and immutable expected snapshots.
Maximum 128 cases / 4096 commands per case / 4096 instructions per script run.
Initial state is observation index 0; each accepted command produces exactly one
additional snapshot. There are no clocks, frame deltas, input-device callbacks,
background threads, automatic random seeds or timing-dependent command reorderings.
Native probe execution has a finite 30-second timeout.

Replay invokes existing portable Emerald APIs. Setup may seed synthetic domains
or a named source-backed checkpoint, but all setup is explicit and is not counted
as gameplay. Unknown commands, invalid arguments, busy script dispatch, rejected
completion, missing source IDs, truncated output and execution errors fail.
No command can directly set a story flag/var and claim it advanced gameplay.
Warp and connection commands resolve only the actual pending source-fixture
event through the existing core transition owner; no arbitrary destination
command is accepted. Every command in a versioned matrix must be declared by
the machine-readable contract; the roster guard pins this boundary.
Typed script completion uses actual pending type/action/sequence; choices are
fixture inputs, not fabricated unconditional success. Bounded pending-host slices
must pin the actual request and explain their host/slice boundary.

I1/I2 may initially execute a deliberately small, explicitly synthetic production
movement fixture. I3 adds source-generated opening/world/progression slices.
Encounter and battle command/domain support is introduced in I5/I6, not claimed
by the initial runner. A required inactive runtime is explicitly null; any attached
runtime must serialize its complete owned state. Verification rejects a case
requiring a domain/command the chosen probe does not implement.

## Canonical snapshot and hashes

Snapshot includes version, source-layout format, domains and explicit observations.
All authoritative persistent bytes are included: SaveBlock2, SaveBlock1 and
PokemonStorage at their active R17 source-layout lengths. Thus world/progression,
flags/vars, party/storage, bag, RTC and VP5 bytes are covered without adding or
migrating persistent fields. Raw byte domains use lowercase hexadecimal strings.

Script state reuses the existing endian-independent versioned checkpoint writer
on a copy: zero inactive stack frames and absent pending-request storage before
serialization. Registry/source identities are pinned separately; resource pointers
are never hashed. Runtime object scalars, encounter runtime and battle runtime
follow their full declared field/array layouts as specified in the JSON contract.
Pokemon values use their authoritative codec; ordered battle events and RNG
state/calls are included. Active runtime serializers require coverage guards
against newly added fields. Never hash sizeof(struct) or raw struct memcmp as a
portable contract.

SHA-256 hashes UTF-8 canonical JSON with sorted keys, compact separators,
ensure_ascii=false, integer/boolean/null/string/array/object values only and one
trailing LF. Each domain gets its own digest; the complete snapshot gets a state
digest. World/objective/pending-request summaries are useful additional diagnostic
observations; they do not substitute for complete persistent/runtime coverage.
No expected digest is automatically generated or blessed by the verifier or CI.
Initial fixture creation records actual probe observations, checks them against
source-backed semantic expectations, then versions the reviewed expectations.

Excluded: padding, addresses, wall clocks, host save counter/slot/status metadata,
UI/audio/camera state, cooked assets and remaster-only preferences.
Save transport is independently checked by R17; gameplay state hash remains stable
across a preserving round-trip. No save compatibility fields are introduced.

## First divergence and negative acceptance

Verification stops at the first failing case/observation and reports case ID,
observation index, the responsible ordered command, first differing domain or
observation field, expected and actual values/digests. Snapshot count/schema and
probe failures are diagnosed, not silently repaired/skipped.
Regression must deliberately alter a command, expected observation/hash and a
persistent byte, then demonstrate detection at the expected first index/domain.
Run the same fixture twice and from a differently located state to rule out
pointer/host-address dependence. Presentation/transport-only metadata must not
change the canonical gameplay snapshot.

## Scope and subsequent owners

I3: real R2 registry and R4 map inputs, R3 provenance, R10 objective derivation.
I4: compiled R17 fixture matrix, optional private real fixture remains private.
I5/I6: representative seeded encounter/battle matrices and R16 accepted deltas.
I7/I8: source manifest/generated package integrity orchestration and corruptions.
I9/I10: smoke marker and deferred Android metadata contracts; no real execution.
T1/V1: deterministic required-dependency full-suite command and actual hosted CI.

This checkpoint certifies no full-game story playthrough, real UE executable,
cooked asset package, physical Android result or BrowserStack result. Existing
R18/environment/runtime stop rules remain unchanged.
Next: R19-I1 — Input replay harness.
