# R6-I2C Player Presentation State Policy

Status: **R6-I2C complete**

This phase classifies every Brendan/May `OBJ_EVENT_GFX` identity without
introducing a second player-state simulation.

## Source truth

Pinned Vanilla+ defines eight player-avatar states per gender through
`sPlayerAvatarGfxIds`:

1. normal;
2. Mach Bike;
3. Acro Bike;
4. surfing;
5. underwater;
6. field move;
7. fishing;
8. watering.

Normal, both bike modes, surfing and underwater are persistent avatar states.
Field move, fishing and watering are transient action graphics.

`OBJ_EVENT_GFX_BRENDAN_DECORATING` and
`OBJ_EVENT_GFX_MAY_DECORATING` exist in the graphics identity catalog but are
not members of `sPlayerAvatarGfxIds`. R6 therefore records them as
`legacy_special_identity` rather than inventing a ninth authoritative avatar
state.

## Representation decision

Brendan and May do **not** get a separate complete 3D character model for every
legacy sprite identity. The ORAS overworld model is the base character; state
differences are represented by animation plus props/accessories/mounts.

| State | Representation | Additional requirement |
| --- | --- | --- |
| normal | base model | on-foot locomotion |
| Mach Bike | base + prop | Mach Bike + bike animation |
| Acro Bike | base + prop | Acro Bike + trick/locomotion animation |
| surfing | base + external mount | surf mount + surf animation |
| underwater | base + accessories | goggles + snorkel; source ShoesA/ShoesB remain candidates until visual validation |
| field move | base + action | field-move action animation |
| fishing | base + prop | fishing rod + fishing animation |
| watering | base + prop | watering can + watering animation |
| decorating | base + action | legacy presentation only; exact decoration prop is deferred |

The R6-I2A archives already contain the Brendan/May base models and the
Goggles, Snorkel, Ring, ShoesA and ShoesB source parts. They do not contain
actual animation clips, and the bike/rod/watering/surf-mount requirements are
not supplied by those player archives.

## Skeleton decision

- Brendan: `player_male`
- May: `player_female`

They remain separate skeleton families. R6-I2B found 44 shared named bones out
of 51, with character-specific bag/hair/head-part bones.

## Authority boundary

The current portable R4 bridge explicitly states that it does not yet expose an
authoritative bike/surf avatar-state model. Therefore this I2C mapping is
**declarative only**. R6 must not inspect meshes, animation state, water tiles,
speed, or presentation components to decide which gameplay state the player is
in.

When the portable gameplay layer exposes authoritative avatar state, R6 may
select the matching presentation record. Until then, normal presentation can be
used as the safe runtime fallback.

## Next step

R6-I2D binds these decisions into
`data/r6/character_presentation_overrides.json` using logical asset IDs only.
No proprietary model paths or binaries belong in the public repository.
