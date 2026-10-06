# R6 Character / NPC Asset and Provenance Contract

Status: **R6-P2 frozen contract**

This document freezes the asset/provenance rules for R6 before implementation
of the character manifest/resolver.

R6 is presentation-only. Emerald/Vanilla+ gameplay state remains authoritative
in R4. Character models, skeletons, animation timing, materials and local asset
availability must never decide gameplay position, collision, visibility,
script results or progression.

## 1. Human character source policy

Primary visual source for R6 human characters is **Pokémon Omega Ruby / Alpha
Sapphire (ORAS)** where a compatible character exists.

Reasons:

- Brendan and May have native ORAS 3D representations;
- major Hoenn NPCs/trainers are represented in the same visual generation;
- overworld character assets use a coherent art style;
- available tooling can read relevant Nintendo 3DS / Game Freak model and
  motion formats;
- using one primary source reduces scale, skeleton and material inconsistency.

Priority order:

1. locally prepared assets from a user-owned ORAS source;
2. verified ORAS-derived reference/export only when needed to identify or
   repair a specific character;
3. secondary compatible source only when ORAS has no usable equivalent;
4. explicit project fallback asset when no verified character asset is ready.

Downloaded community archives are reference/fallback sources, not authoritative
proof of redistribution rights.

Known useful tooling/reference surfaces include:

- `sxrmss/n3ds_importer` for GFModel/GFMotion/BCH import;
- Ohana3DS / Ohana3DS Rebirth as optional inspection/extraction tools;
- The Models Resource as an identification/reference surface for already
  catalogued ORAS characters.

No extraction key, DRM-bypass material, ROM image or commercial source payload
belongs in the public repository.

## 2. Public repository vs private/local asset boundary

The public repository may contain:

- graphics-ID -> presentation-ID mappings;
- manifest builders and validators;
- hashes;
- provenance metadata;
- scale/ground/rotation metadata;
- skeleton-family metadata;
- animation semantic names;
- import settings;
- redistributable placeholders;
- tests and audit reports.

The public repository must not contain:

- extracted ORAS models;
- extracted textures;
- extracted animations;
- ROM/CIA/3DS files or equivalent commercial source containers;
- Unreal assets generated from extracted commercial payloads;
- private absolute filesystem paths.

The companion private repository
`illetyus/pokemon-emerald-remastered-assets` may be used as a personal asset
vault, but **public project code must not depend on that GitHub repository URL
or on GitHub availability**.

Runtime/build tooling resolves assets from a local asset root. The existing
public-repository ignores for `/local/` and `/generated/` remain the
canonical local-output boundary.

Recommended local layout:

```text
local/r6/
  source/
    oras/
      characters/
      textures/
      animations/
  normalized/
    characters/
    textures/
    animations/

generated/r6/
  manifest/
  unreal/
```

The private asset repository may mirror this logical structure for backup and
personal synchronization.

## 3. Presentation manifest contract

R6-I1 will implement the manifest/resolver, but the record shape is frozen here.

Each character/object presentation record must contain these logical fields:

```text
schema_version
graphics_id
graphics_name
presentation_id
presentation_kind
source_family
skeleton_family
model_id
material_ids[]
animation_set_id
scale
ground_offset_cm
yaw_offset_deg
lod_profile
fallback_id
provenance_id
source_sha256[]
normalized_sha256[]
```

Field rules:

- `graphics_id`: authoritative numeric Emerald/Vanilla+ graphics identity.
- `graphics_name`: symbolic source identity such as
  `OBJ_EVENT_GFX_PROF_BIRCH`.
- `presentation_id`: stable project-owned visual identity.
- `presentation_kind`: one of `human`, `pokemon_overworld`,
  `special_object`, or `fallback`.
- `source_family`: e.g. `oras`, `project_placeholder`, or an explicitly
  named secondary source.
- `skeleton_family`: project retarget family; never gameplay data.
- `model_id`, `material_ids`, `animation_set_id`: logical IDs, not
  absolute asset paths.
- scale/ground/yaw values are presentation-only.
- `fallback_id` is mandatory whenever the primary local asset is optional.
- hashes are SHA-256 lowercase hexadecimal values.

The public manifest never stores a user's machine-specific path.

## 4. Initial human skeleton families

R6 uses shared animation families where practical, but never forces unlike
source skeletons into one hierarchy merely to reduce file count.

Initial families:

- `player_male`
- `player_female`
- `adult_male`
- `adult_female`
- `teen_male`
- `teen_female`
- `child`
- `elderly`
- `large_human`
- `special_human`

Brendan and May remain separate player families because their equipment and
special states differ from generic NPCs.

A character may be assigned to a family only after its source skeleton and
retarget result are verified. If a source skeleton cannot safely share the
family rig, the character keeps its own skeleton while retaining the family
metadata for fallback animation policy.

R6-P2 does **not** require destructive skeleton merging.

## 5. Animation semantic contract

R6 character animation identities are presentation semantics:

- `idle`
- `walk`
- `run`
- `turn`
- `interact`

Later implementation may add state-specific variants such as bike, surf,
fishing or watering, but the base semantic names above remain stable.

Animation completion must never determine authoritative movement or script
completion. Presentation follows R4/R2 state/events.

## 6. Player state policy

Emerald exposes several graphics identities for Brendan and May, including
normal, bike, surfing, field move, underwater, fishing, watering and decorating
states.

R6 must map these source identities to presentation states without creating
duplicate gameplay state in Unreal.

Examples:

```text
OBJ_EVENT_GFX_BRENDAN_NORMAL -> player.brendan.normal
OBJ_EVENT_GFX_MAY_NORMAL     -> player.may.normal
```

The resolver may choose a shared base model plus equipment/state overlays where
that is visually correct; it does not require one separate full model per
legacy sprite identity.

## 7. Named NPC and generic NPC policy

Named Hoenn characters use their closest verified ORAS counterpart when
available.

Examples include Professor Birch, Roxanne, Brawly, Wattson, Flannery, Norman,
Winona, Liza/Tate, Wallace, Steven, Wally, Archie, Maxie, Sidney, Phoebe,
Glacia, Drake and the player's mother.

Generic Emerald NPC/trainer classes are mapped to ORAS human families where a
verified counterpart exists. R6-I3 owns the detailed class mapping.

A generic NPC may share a model with other compatible source identities when
that is an intentional mapping recorded in the manifest.

A named character must **never silently resolve to another named character**.

## 8. Missing-asset fallback rules

Missing local commercial assets must not block gameplay.

Fallback order:

1. exact presentation asset;
2. verified family-compatible presentation asset only when the manifest
   explicitly allows it;
3. project-owned family placeholder;
4. project-owned generic visible placeholder.

Fallbacks must be visually obvious during development and auditable.

The following are forbidden:

- silently hiding a required visible NPC because its model is missing;
- silently substituting another named story character;
- using asset availability to change collision, interaction, scripts or flags.

## 9. Pokémon-overworld boundary with R14

Some Emerald object graphics identities represent Pokémon in the overworld.

R6 records these identities as `pokemon_overworld`, but does not create a
second independent Pokémon model pipeline.

Until R14 is implemented they use an explicit placeholder/fallback.
R14 later becomes the source of actual Pokémon presentation assets and R6
consumes that shared species/presentation resolution where appropriate.

This prevents duplicate model ownership between R6 and R14.

## 10. Provenance records

Every non-placeholder local asset group must have a provenance record containing
at least:

```text
provenance_id
source_family
source_title
source_identity
source_locator_or_archive_identity
preparation_method
tool_names_and_versions
source_sha256[]
normalized_sha256[]
notes
```

Rules:

- hashes refer to the exact bytes used;
- provenance contains no secret, key or personal absolute path;
- replacing source bytes without updating the hash/provenance is an error;
- a changed normalized output requires a changed normalized hash;
- community download metadata must not be represented as a license grant unless
  an actual applicable license is verified.

## 11. Deterministic generation rules

Given identical mapping inputs, local asset metadata and tool versions, R6
manifest generation must be deterministic.

Required rules:

- sort records by numeric `graphics_id`, then symbolic `graphics_name`;
- stable UTF-8 output with LF line endings;
- no generated timestamps in canonical manifest bytes;
- no machine-specific absolute paths;
- duplicate numeric identities are rejected unless the source contract
  explicitly defines conditional alternatives;
- duplicate `presentation_id` definitions with conflicting metadata are
  rejected;
- referenced hashes use SHA-256;
- every unresolved required identity must have an explicit fallback;
- manifest generation must not inspect or modify gameplay state.

## 12. R4 authority boundary

The character asset system may consume:

- object graphics identity;
- authoritative object visibility;
- authoritative object position/elevation;
- authoritative facing/movement presentation state when exposed by R4;
- player gender/state exposed by the core.

It may not own or recompute:

- collision;
- object activation;
- object gameplay coordinates;
- trainer sight;
- scripts;
- flags/vars;
- warps;
- story state.

The R6 presentation snapshot required to render moving NPCs must therefore be a
read-only view of R4 state, not a second NPC simulation.

## 13. R6-P2 exit decision

R6-P2 is complete when this contract is accepted as the implementation boundary:

- ORAS is the primary human character source;
- commercial payloads remain private/local;
- public manifest uses logical IDs and hashes only;
- skeleton families and fallback policy are explicit;
- provenance is mandatory;
- deterministic generation rules are fixed;
- overworld Pokémon presentation is delegated to the shared R14 asset pipeline;
- gameplay authority remains in R4.

Next subphase: **R6-I1 — Character manifest/resolver**.
