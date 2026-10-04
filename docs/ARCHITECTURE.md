# Architecture

## Production decision

The production runtime architecture is fixed:

**portable deterministic C/C++ gameplay core + Unreal Engine presentation/platform host.**

The earlier SDL3 and Godot implementations remain historical R0
experiments/reference evidence only. They are not production candidates.

The canonical remaining phase order is defined in
[ROADMAP.md](ROADMAP.md).

## Goal

Preserve authoritative Pokémon Emerald/Vanilla+ gameplay behavior while
allowing rendering, animation, camera, UI, audio, input and platform
integration to be replaced independently.

## Authoritative sources

Gameplay behavior is derived from:

1. pinned Emerald/Vanilla+ source behavior;
2. deterministic converted data packages;
3. accepted project regressions and deliberate Vanilla+ deltas.

Secondary wikis may help explain behavior, but they are not the gameplay
specification.

## Layers

### 1. Gameplay core

Owns gameplay truth:

- map identity and player tile position;
- collision results;
- object-event state;
- scripts;
- flags and vars;
- warps and map connections;
- story/progression state;
- quest/objective derivation;
- Pokémon/party/storage domain;
- items/bag;
- encounters;
- battle rules and deterministic results;
- save-domain state;
- gameplay RNG ordering.

The core must not depend on Unreal objects, Android UI, touch coordinates,
rendering APIs, audio assets or frame rate.

### 2. Converted game data

Source data is converted into deterministic, audited runtime packages.

Examples:

- Hoenn map/layout catalog;
- object events;
- scripts;
- encounter data;
- tileset/render descriptors;
- later Pokémon/item/battle catalogs.

Conversion does not move gameplay authority into presentation.

### 3. Presentation

Unreal consumes explicitly exported gameplay state and one-way presentation
events.

Presentation owns:

- world visuals;
- character/Pokémon models;
- animation;
- camera;
- lighting;
- particles/VFX;
- battle presentation;
- HUD/menus;
- map/quest display;
- audio playback;
- visual transitions.

Presentation may request gameplay actions through the command/input boundary.
It must not directly mutate authoritative gameplay state.

### 4. Platform

Provides target-specific services:

- touch/gamepad/keyboard input;
- clock/RTC adapter;
- filesystem/save access;
- Android lifecycle;
- display/window;
- audio device;
- haptics;
- device profiles/scalability.

## Core invariant

Given the same:

- initial state;
- converted data set;
- RNG seed/state;
- ordered gameplay inputs;

the gameplay core must produce the same authoritative gameplay-state sequence.

Frame rate, rendering quality, animation timing, audio timing or Android device
must not alter gameplay results.

## Command and event boundary

Input flows toward the core as explicit actions/commands.

Examples:

- move up/down/left/right;
- interact;
- menu-confirmed gameplay action;
- battle selection.

The core returns:

1. current authoritative state/snapshots; and
2. descriptive one-way events about what happened.

Examples include:

- moved;
- blocked;
- interacted;
- warp/map change;
- encounter started;
- item changed;
- quest/objective changed;
- battle started;
- move used;
- damage applied;
- critical hit;
- status changed;
- faint;
- battle ended.

Presentation events may drive animation, camera, VFX, haptics and audio.
Handling an event must never create or alter the gameplay result it describes.

## World authority

Emerald world data remains authoritative.

A modern 3D environment does not replace Emerald map geometry.

The intended relationship is:

```text
Emerald map/layout/collision/warp/object truth
                    +
presentation-only 3D visual assets
```

A building mesh, tree mesh, water surface or camera obstruction must not become
the gameplay collision source.

## Asset factory boundary

Character, Pokémon, environment and audio assets use a local preparation
pipeline:

```text
user-owned source data
        ↓
extract
        ↓
normalize / convert
        ↓
validate
        ↓
manifest + hashes
        ↓
Unreal-ready local package
```

The public repository may contain tooling, mappings, manifests, hashes,
provenance, validators and redistributable placeholders.

It must not contain extracted commercial ROMs, models, textures or audio.

The asset factory is presentation infrastructure. It never becomes gameplay
authority.

## Character/NPC presentation

R6 maps authoritative Emerald object/graphics identities to presentation
assets.

- gameplay object position/state stays in the core;
- shared human animation families may be used;
- retargeting may reduce duplicate animation work;
- missing models must have an explicit fallback;
- model availability cannot block authoritative gameplay simulation.

## Environment presentation

R7 maps world visual identities to 3D assets.

- Emerald layouts remain unchanged unless a deliberate gameplay change is
  separately accepted;
- ORAS-compatible assets may be used as local visual source material;
- full ORAS maps are not substituted for Emerald maps;
- mobile-oriented LOD/HLOD/instancing is required;
- Lumen is optional, never required for gameplay or basic visual correctness.

## Pokémon battle presentation

R13 owns battle results.

R14 consumes battle events and resolves species to local presentation assets.

Animation timing, VFX completion or camera state must not control battle
resolution.

Some source Pokémon assets may use material/visibility animation in addition to
skeletal animation. The asset pipeline must report unsupported/missing cases
explicitly.

## Audio boundary

Gameplay/presentation events expose semantic music/SFX/jingle/cry identities.

The Unreal audio system resolves those identities to local audio assets.

Core gameplay code must not contain Unreal asset paths.

The same semantic ID may resolve to an original-style local pack or an
optional remastered pack without changing gameplay.

## Android-first rendering

Android is the primary target.

Therefore:

- mobile rendering budgets are designed in from the start;
- performance is not solved by building a desktop scene first and reducing it
  at the end;
- repeated environment assets should use efficient instancing where practical;
- expensive rendering features are optional scalability tiers;
- actual performance targets are finalized only after R18 real UE/Android
  builds and device measurements.

## Build boundary

Before R18:

- portable core tests run;
- data conversion/audits run;
- Unreal source architecture can be statically validated;
- no real UE executable/APK success is claimed.

At R18:

- Unreal Engine 5.8.3 is installed/verified on the project PC;
- production Unreal code is compiled;
- Android is cooked/packaged;
- real runtime smoke starts.

BrowserStack real-device execution occurs only after a valid APK exists.

## Security

The repository is public.

- no ROMs;
- no extracted commercial game assets;
- no credentials;
- no BrowserStack/Doppler secrets;
- generated local proprietary asset outputs stay ignored;
- a future self-hosted runner must not execute untrusted public-fork PR code
  with host or secret access.

## Historical architecture experiments

The R0 SDL3 and Godot experiments are preserved as evidence and regression
references where useful.

They do not override the Unreal production decision recorded in
`docs/adr/0001-unreal-engine-production-runtime.md`.
