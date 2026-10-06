# R13 — Battle Core Completion Evidence

Date: 2026-10-06  
Branch: `r13-battle-core`  
PR: #13 — R13: portable deterministic battle core  
Acceptance implementation head before final documentation: `f6e051b8240d08a81fc19fa772db1513f8fb0eef`

## Status

**R13 COMPLETE at portable-core/source-host acceptance level.**

R13 ports Emerald/Vanilla+ battle authority into the portable deterministic core.
Unreal remains presentation/input only and does not calculate battle outcomes.

The ownership model is:

```text
R11 Pokémon / party / item state
        +
R12 deterministic encounter result
        ↓
R13 portable battle state + Emerald RNG
        ↓
authoritative battle events + resulting Pokémon/save state
        ↓
Unreal presentation only
```

## Source truth

The primary behavioral specification is the pinned `vendor/vanillaplus` source,
including:

- `src/battle_main.c`;
- `src/battle_util.c`;
- `src/battle_script_commands.c`;
- `src/battle_ai_script_commands.c`;
- `src/battle_ai_switch_items.c`;
- `src/battle_setup.c`;
- `src/pokemon.c`;
- trainer and trainer-party source tables;
- battle AI scripts and battle constants.

The R13 Python source audit pins the portable implementation to the relevant
Emerald/Vanilla+ control-flow and data shapes.

## Deterministic battle state and RNG

R13 owns:

- battle state and battler state;
- ordered battle actions;
- descriptive battle events;
- Emerald LCRNG state and call count;
- deterministic fixed-seed replay.

Turn-order parity includes:

- move priority;
- effective speed;
- exact-speed RNG tie breaking;
- Quick Claw using one shared per-turn random value, matching
  `gRandomTurnNumber`;
- ITEM and SWITCH actions remaining in battler order instead of entering the
  move speed/priority sort;
- Safari action ordering;
- link/non-link run ordering.

## Move execution and battle mechanics

Portable coverage includes the battle mechanics used by the generated Emerald
move catalog, including:

- Gen III type effectiveness;
- accuracy/evasion;
- critical-hit calculation;
- physical/special Gen III type split;
- status and volatile state;
- weather;
- screens and side state;
- common battle abilities;
- held-item effects;
- multi-hit and multi-turn state;
- switching and Baton Pass state transfer;
- Pursuit interception;
- PP consumption and Pressure;
- deterministic called-move behavior.

The source audit rejects any move effect used by the generated move catalog
that lacks a portable handler.

## Trainer AI

Trainer move AI follows the Vanilla+ score-script pipeline in source order:

```text
CHECK_BAD_MOVE
→ TRY_TO_FAINT
→ CHECK_VIABILITY
→ SETUP_FIRST_TURN
→ RISKY
→ PREFER_POWER_EXTREMES
→ PREFER_BATON_PASS
→ DOUBLE_BATTLE
→ HP_AWARE
→ TRY_SUNNY_DAY_START
```

Switch/item AI additionally covers:

- Perish Song switching;
- Wonder Guard switching;
- absorb-ability switching;
- Natural Cure;
- trapping checks;
- immune/resistant reserve selection;
- suitable reserve selection;
- healing/status-cure items;
- X-stat / Dire Hit;
- Guard Spec.

The currently generated trainer data uses Potion, Super Potion, Hyper Potion
and Full Restore, while the portable item classifier preserves the broader
Emerald AI item classes.

## Faint, switch and replacement lifecycle

Lifecycle regressions pin:

- faint-state cleanup;
- Spikes-before-entry-ability ordering;
- doubles replacement slots never colliding on one reserve;
- replacement phases not incrementing the normal turn counter;
- Pursuit resolving against the outgoing Pokémon before switch;
- a battler fainted earlier in a turn losing its queued action;
- later moves retargeting from a fainted opponent to the surviving opposing
  partner;
- Battle Arena rejecting voluntary switching.

Progression ordering is explicitly regression-tested as:

```text
FAINT
→ EXP
→ LEVEL_UP
→ MOVE_LEARN handoff
→ replacement phase SWITCH
```

The replacement phase remains separate and does not advance the normal turn
counter.

## Battle-end progression

R13 propagates battle EXP and level changes into portable Pokémon state.

Move learning is emitted as a handoff event rather than silently accepting a
move choice.

Evolution is also a handoff. It is deferred until a won battle has ended,
matching Emerald's post-battle evolution lifecycle. A loss does not emit the
post-battle evolution handoff.

## Wild run, capture and special lifecycle

Wild capture includes the Emerald ball path and the accepted Vanilla+ Ultra
Ball 4× multiplier.

Escape lifecycle regressions cover:

- a FIRST battle rejecting ordinary escape;
- Safari run ending immediately;
- Battle Frontier trainer run resolving as a forfeit before ability/item
  escape shortcuts;
- ordinary trainer battles rejecting Run Away rather than letting the ability
  bypass trainer-battle rules;
- player-side link run resolving to the final local loss outcome;
- non-player run actions resolving to `MON_FLED`, with Emerald's
  wrap/escape-prevention failure path;
- equal-speed single battles escaping without consuming RNG;
- a dedicated `run_tries` counter rather than using battle turn count;
- Emerald's `u8 speedVar` truncation/overflow semantics for escape checks;
- ordinary doubles not using the single-battle speed escape check;
- Run Away / always-run held-item ordering relative to field trapping;
- Battle Pyramid floor-template run multipliers derived from the original
  SaveBlock2 `curChallengeBattleNum` and `pyramidRandoms[3]` fields;
- Battle Pyramid's special Run Away ordering, where `runTries` increments
  before the multiplier/RNG formula;
- field trapping checks.

Special-mode lifecycle also includes the explicitly represented Arena, Safari,
Link/Recorded Link and Frontier/Pyramid gating in the portable battle core.
Facility world/challenge orchestration, presentation and later platform/runtime
integration remain outside R13.

## Save-domain and presentation boundary

R13 consumes R11 Pokémon state and commits resulting player-party state through
the portable save-domain API.

Trainer completion propagates battle results and reward state through the core.

Unreal source consumes the battle contract only. The R13 boundary audit verifies
that presentation code does not own battle math or outcome calculation.

## Regression evidence

Acceptance implementation head:
`f6e051b8240d08a81fc19fa772db1513f8fb0eef`.

Dedicated R13 run:

- R13 Battle #341 — run `37419838436` — SUCCESS.

The job passed:

- Configure;
- Build R13 targets;
- Run R13 regressions;
- Audit pinned Vanilla+ battle contract;
- Validate Unreal source boundary.

Same-head earlier-phase regression gates:

- R0 Core #1201 — run `37419838464` — SUCCESS;
- R4 Overworld #298 — run `37419838434` — SUCCESS;
- R5 World Renderer #319 — run `37419838466` — SUCCESS;
- R10 Quest Map #216 — run `37419838612` — SUCCESS;
- R11 Pokémon / Party / Item #216 — run `37419838493` — SUCCESS;
- R12 Encounter #215 — run `37419838427` — SUCCESS.

PR #13 is open and mergeable on the acceptance implementation head.

## R13 exit checklist

- [x] deterministic Emerald battle RNG;
- [x] single/double battle initialization;
- [x] turn/action ordering;
- [x] ITEM/SWITCH ordering parity;
- [x] shared per-turn Quick Claw RNG semantics;
- [x] damage / accuracy / critical / type-effectiveness path;
- [x] status, weather, ability and held-item behavior;
- [x] generated move-effect coverage audit;
- [x] trainer move AI pipeline;
- [x] trainer switch/item AI;
- [x] Pursuit switch interception;
- [x] faint-state cleanup;
- [x] distinct doubles replacement slots;
- [x] same-turn fainted-action cancellation and retargeting;
- [x] replacement phase does not advance the normal turn counter;
- [x] progression/faint/replacement ordering;
- [x] post-battle evolution handoff ordering;
- [x] special escape lifecycle;
- [x] Battle Pyramid save-derived escape multiplier and Run Away semantics;
- [x] foe `MON_FLED` run lifecycle;
- [x] wild capture;
- [x] player-party/save result propagation;
- [x] deterministic replay coverage;
- [x] Vanilla+ source audit;
- [x] Unreal presentation-only boundary;
- [x] R0/R4/R5/R10/R11/R12/R13 required CI gates green.

**R13 exit gate: PASSED.**

## Next phase

R16 — Vanilla+ QoL.

R16 should begin only after PR #13 is merged to `main`, per the canonical
roadmap and repository policy.
