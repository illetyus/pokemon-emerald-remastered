# Vanilla+ Phase 10A — Quest Tracking and Navigation

Date: 2026-10-02
Branch: `phase10a-quest-navigation`
Version owner: `VP018 / T018 / V+018`

## Design principle

Phase 10A does not create a second persistent story state. It derives the current objective from Emerald's existing flags and vars, then presents that state through a minimal quest UI and navigation layer.

`Emerald world state -> objective resolver -> quest UI -> region target -> local target`

No SaveBlock or PokemonStorage layout changes are allowed.

## Guidance model

- Quest UI answers: what should I do?
- Region Map answers: which area should I reach?
- Local marker answers: which NPC/object matters after arrival?
- Only one main-story objective is highlighted at a time.
- Navigation identifies the destination but does not calculate or draw a GPS-style route.

## Delivery slices

1. Vertical slice: early story through Devon/Steven/Slateport.
2. Region-map direct-open and objective targeting.
3. Persistent local target marker for object events / coordinates.
4. Expand objective table through Champion.
5. Regression, old-save inference and field test.


## Implemented main-story coverage

The derived objective table now spans the main campaign from the Route 103 rival objective through Devon/Steven/Slateport, all eight Gyms, Meteor Falls / Mt. Chimney, Weather Institute, Mt. Pyre, Magma and Aqua hideouts, Mossdeep Space Center, Seafloor Cavern, Sootopolis / Sky Pillar, Victory Road, and the Pokemon League.

The final objective completes when `FLAG_SYS_GAME_CLEAR` is set. No quest progress is stored separately in save data.
