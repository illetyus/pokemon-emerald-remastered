# R6-I3A NPC / Trainer Source Coverage Inventory

Status: R6-I3A complete.

R6-I3A covers the 121 non-player human graphics identities that remain after the 18 Brendan/May player identities closed in R6-I2.

## Public ORAS source coverage

The current Pokemon Omega Ruby / Alpha Sapphire page on The Models Resource reports 295 total assets. Human-oriented sections relevant to R6 are:

- 2 playable-character packages
- 6 playable-character overworld packages
- 5 unique-NPC packages
- 33 unique-NPC overworld packages
- 50 trainer-class packages

No separate generic-overworld-NPC section is present. Emerald identities such as BOY_1, WOMAN_1, OLD_MAN, NURSE and similar generic map people therefore cannot be assumed to have a ready public package.

## Conservative mapping policy

R6-I3A distinguishes five cases:

- oras_public_exact: unambiguous named/class counterpart is publicly catalogued
- oras_public_combined_needs_inspection: package likely contains multiple relevant identities and must be inspected
- reuse_verified_player_base: rival/link Brendan or May reuses the verified R6-I2 player base
- oras_extract_or_family_map_required: inspect owned ORAS data or map to a verified shared family
- secondary_source_required: legacy/non-ORAS case needs another source decision

Status counts:

- oras_extract_or_family_map_required: 45
- oras_public_combined_needs_inspection: 12
- oras_public_exact: 39
- reuse_verified_player_base: 12
- secondary_source_required: 13

## Named Hoenn coverage

Direct ORAS overworld packages are identified for Professor Birch, Roxanne, Brawly, Wattson, Flannery, Norman, Winona, Wallace, Steven, Wally, Archie, Maxie, the player's mother, Sidney, Phoebe, Glacia and Drake.

Liza and Tate remain a combined package pending component inspection.

Team Aqua/Magma grunts and a substantial set of Emerald trainer classes also have direct ORAS trainer-class packages.

## Generic NPC gap

Generic Emerald NPC graphics are not force-matched to arbitrary trainer classes. Preferred path:

1. inspect user-owned ORAS character data
2. import actual model/skeleton/texture data with a compatible ORAS reader
3. classify the result into R6 skeleton families
4. share a family model only after visual verification

The current sxrmss/n3ds_importer project supports Pokemon X/Y, ORAS, Sun/Moon and Ultra Sun/Ultra Moon GFModel/GFMotion/BCH data, including skeletons, skin weights, materials, textures and skeletal animation import. It ships no game data.

## Legacy / secondary-source cases

Battle Frontier brains, Juan, Scott, Red/Leaf, Brandon, and the legacy R/S link outfits are not silently substituted with unrelated ORAS characters. They remain explicitly deferred until a secondary source is chosen.

## Next slice

R6-I3B - NPC asset acquisition and package inspection.

Download the exact/combined ORAS NPC and trainer packages selected by this inventory into the private asset repository, hash them, record provenance, and inspect combined packages before any manifest binding.
