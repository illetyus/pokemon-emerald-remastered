#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UNREAL = ROOT / "unreal"
MODULE = UNREAL / "Source" / "PokemonEmeraldRemastered"


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def main() -> int:
    errors: list[str] = []

    project = json.loads(
        (UNREAL / "PokemonEmeraldRemastered.uproject").read_text(encoding="utf-8")
    )
    require(
        project.get("EngineAssociation") == "5.8",
        "uproject must target Unreal Engine 5.8",
        errors,
    )

    # R20: retain the original R0 visual/input reference outside the runtime module.
    for filename in ("R0HUD.h", "R0HUD.cpp", "R0PlayerController.h", "R0PlayerController.cpp"):
        require(not (MODULE / filename).exists(),
                f"historical R0 presentation must not compile in production: {filename}", errors)
        require((ROOT / "experiments" / "unreal-r0-reference" / filename).is_file(),
                f"missing preserved R0 reference: {filename}", errors)

    expected_embeds = {
        "RemasterCoreEmbed.cpp": "../../../core/src/core.c",
        "RemasterPlatformEmbed.cpp": "../../../core/src/platform.c",
        "RemasterMechanicsEmbed.cpp": "../../../core/src/mechanics.c",
        "RemasterEmeraldSaveEmbed.cpp": "../../../core/src/emerald_save.c",
        "RemasterEmeraldRtcEmbed.cpp": "../../../core/src/emerald_rtc.c",
        "RemasterEmeraldStateEmbed.cpp": "../../../core/src/emerald_state.c",
        "RemasterEmeraldQuestEmbed.cpp": "../../../core/src/emerald_quest.c",
        "RemasterEmeraldPokemonEmbed.cpp": "../../../core/src/emerald_pokemon.c",
        "RemasterEmeraldItemsEmbed.cpp": "../../../core/src/emerald_items.c",
        "RemasterEmeraldQolEmbed.cpp": "../../../core/src/emerald_qol.c",
        "RemasterEmeraldEncounterEmbed.cpp": "../../../core/src/emerald_encounter.c",
        "RemasterEmeraldMapEmbed.cpp": "../../../core/src/emerald_map.c",
        "RemasterEmeraldMovementEmbed.cpp": "../../../core/src/emerald_movement.c",
        "RemasterEmeraldEventsEmbed.cpp": "../../../core/src/emerald_events.c",
        "RemasterEmeraldTransitionEmbed.cpp": "../../../core/src/emerald_transition.c",
        "RemasterEmeraldScriptEmbed.cpp": "../../../core/src/emerald_script.c",
        "RemasterEmeraldScriptRuntimeEmbed.cpp": "../../../core/src/emerald_script_runtime.c",
        "RemasterEmeraldObjectStateEmbed.cpp": "../../../core/src/emerald_object_state.c",
    }

    for filename, include in expected_embeds.items():
        path = MODULE / filename
        require(path.is_file(), f"missing Unreal embed: {filename}", errors)
        if path.is_file():
            text = path.read_text(encoding="utf-8")
            require(include in text, f"{filename} does not embed {include}", errors)
            require('extern "C"' in text, f"{filename} must use C linkage", errors)

    build = (MODULE / "PokemonEmeraldRemastered.Build.cs").read_text(encoding="utf-8")
    for dependency in (
        "Core",
        "CoreUObject",
        "Engine",
        "InputCore",
        "Json",
        "UMG",
        "EnhancedInput",
        "Niagara",
        "DeveloperSettings",
    ):
        require(
            f'"{dependency}"' in build,
            f"Build.cs missing dependency {dependency}",
            errors,
        )

    engine_ini = (UNREAL / "Config" / "DefaultEngine.ini").read_text(encoding="utf-8")
    for expected in (
        "PackageName=com.illetyus.emeraldremaster.r0unreal",
        "TargetSDKVersion=35",
        "bBuildForArm64=True",
        "Orientation=Landscape",
    ):
        require(expected in engine_ini, f"DefaultEngine.ini missing {expected}", errors)

    lock = json.loads((ROOT / "external" / "sources.lock.json").read_text(encoding="utf-8"))
    require(
        lock.get("vanillaplus", {}).get("commit")
        == "70db90c9077aed1272e746fc2537d9f12b95a91c",
        "Vanilla+ production source pin changed unexpectedly",
        errors,
    )

    required_runtime_files = [
        "RemasterCoreAdapter.cpp",
        "RemasterCoreSubsystem.cpp",
        "RemasterPlatformUnreal.cpp",
        "RemasterWorldData.cpp",
        "RemasterWorldActor.cpp",
        "RemasterCameraRig.cpp",
        "RemasterEnvironmentController.cpp",
        "RemasterUISubsystem.cpp",
        "RemasterNavigationSubsystem.cpp",
        "RemasterPlayerController.cpp",
        "RemasterBattlePresentationSubsystem.cpp",
        "RemasterBattleStage.cpp",
        "RemasterFeedbackSubsystem.cpp",
        "RemasterAudioSubsystem.cpp",
        "RemasterPerformanceSubsystem.cpp",
        "RemasterVanillaPlusSaveSubsystem.cpp",
        "RemasterWorldGameplaySubsystem.cpp",
        "RemasterRenderCatalogSubsystem.cpp",
        "RemasterRenderCatalogSubsystem.cpp",
        "RemasterRenderResourceSubsystem.cpp",
        "RemasterOverworldPawn.cpp",
        "RemasterCharacterCatalogSubsystem.cpp",
        "RemasterCharacterVisualComponent.cpp",
        "RemasterCharacterPresentationRead.h",
        "RemasterCharacterAssetSet.h",
        "RemasterNpcPresentationWorld.cpp",
    ]
    for filename in required_runtime_files:
        require((MODULE / filename).is_file(), f"missing runtime layer: {filename}", errors)

    navigation_h = (
        MODULE / "RemasterNavigationSubsystem.h"
    ).read_text(encoding="utf-8")
    navigation_cpp = (
        MODULE / "RemasterNavigationSubsystem.cpp"
    ).read_text(encoding="utf-8")
    require(
        "bool RefreshFromCore()" in navigation_h
        and "SetActiveObjective" not in navigation_h
        and "ClearActiveObjective" not in navigation_h,
        "R10 navigation must derive objectives from core instead of accepting Unreal-authored progression",
        errors,
    )
    require(
        '#include "remaster/emerald_quest.h"' in navigation_cpp
        and "remaster_emerald_quest_active(Save)" in navigation_cpp
        and "GetNativeSaveHandle()" in navigation_cpp,
        "R10 navigation must consume the portable quest resolver and authoritative save",
        errors,
    )
    require(
        "OnGameplayMapChanged.AddDynamic(" in navigation_cpp
        and "HandleGameplayMapChanged(" in navigation_cpp,
        "R10 navigation must refresh presentation context after authoritative map changes",
        errors,
    )
    require(
        "remaster_emerald_flag_set(" not in navigation_cpp
        and "remaster_emerald_var_set(" not in navigation_cpp,
        "R10 navigation presentation must never mutate Emerald story flags/vars",
        errors,
    )

    world_gameplay_h = (
        MODULE / "RemasterWorldGameplaySubsystem.h"
    ).read_text(encoding="utf-8")
    world_gameplay_cpp = (
        MODULE / "RemasterWorldGameplaySubsystem.cpp"
    ).read_text(encoding="utf-8")
    require(
        "FRemasterWildEncounterPresentation" in world_gameplay_h
        and "OnWildEncounterGenerated" in world_gameplay_h
        and "NativeEncounterRuntime" in world_gameplay_h,
        "R12 world host must expose presentation-only encounter results and hold opaque portable runtime state",
        errors,
    )
    require(
        '#include "remaster/emerald_encounter.h"' in world_gameplay_cpp
        and "remaster_emerald_encounter_runtime_init(" in world_gameplay_cpp
        and "remaster_emerald_encounter_step(" in world_gameplay_cpp
        and "ProcessEncounterAfterMove(" in world_gameplay_cpp,
        "R12 moved-step adapter must call the portable encounter runtime",
        errors,
    )
    require(
        "Phase9ChooseWildSpecies" not in world_gameplay_cpp
        and "Random()" not in world_gameplay_cpp
        and "encounterRate *=" not in world_gameplay_cpp,
        "R12 Unreal host must not reimplement species/RNG/encounter-rate authority",
        errors,
    )
    require(
        "void SetEncounterSeed(uint32 Seed);" in world_gameplay_h
        and 'UFUNCTION(BlueprintCallable, Category="Remaster|World|Gameplay")\n    void SetEncounterSeed' not in world_gameplay_h,
        "R12 encounter RNG seed injection must remain host-only and unavailable to Blueprint presentation",
        errors,
    )
    require(
        "remaster_emerald_encounter_restart_immunity(" in world_gameplay_cpp
        and "remaster_emerald_encounter_roamer_move(" in world_gameplay_cpp
        and "remaster_emerald_encounter_roamer_warp(" in world_gameplay_cpp,
        "R12 map transitions must preserve Vanilla immunity and roamer transition semantics",
        errors,
    )

    require(
        (MODULE / "RemasterEmeraldPokemonEmbed.cpp").is_file()
        and (MODULE / "RemasterEmeraldItemsEmbed.cpp").is_file(),
        "R11 Pokémon/item portable cores must be embedded for Unreal builds",
        errors,
    )

    # Guard against accidentally keying visuals by full map words. Visual
    # selection must use decoded metatile IDs while collision/elevation remain
    # gameplay/domain data.
    world_actor = (MODULE / "RemasterWorldActor.cpp").read_text(encoding="utf-8")
    world_gameplay = (
        MODULE / "RemasterWorldGameplaySubsystem.cpp"
    ).read_text(encoding="utf-8")
    world_gameplay_h = (
        MODULE / "RemasterWorldGameplaySubsystem.h"
    ).read_text(encoding="utf-8")
    require(
        "LoadedMap.MetatileIds[Index]" in world_actor,
        "world renderer must select visuals by decoded metatile id",
        errors,
    )
    require(
        "LoadedMap.RawBlocks[Index]" not in world_actor,
        "world renderer must not use raw map words as visual IDs",
        errors,
    )

    # R5 renderer must consume the authoritative gameplay map and keep
    # presentation coordinates/chunking separate from gameplay collision.
    world_actor_h = (
        MODULE / "RemasterWorldActor.h"
    ).read_text(encoding="utf-8")
    require(
        "bool LoadAuthoritativeMap()" in world_actor_h
        and "GetCurrentMapForPresentation()" in world_actor,
        "R5 world renderer must consume the authoritative gameplay map",
        errors,
    )
    require(
        "OnGameplayMapChanged.AddDynamic(" in world_actor
        and "HandleGameplayMapChanged(" in world_actor,
        "R5 renderer must rebuild from authoritative map-transition events",
        errors,
    )
    require(
        '#include "RemasterWorldGridMath.h"' in world_actor
        and "remaster::world_grid::chunk_for_tile(" in world_actor
        and "remaster::world_grid::tile_axis_to_local(" in world_actor
        and "remaster::world_grid::local_axis_to_tile(" in world_actor,
        "R5 renderer must use the tested shared tile/chunk coordinate math",
        errors,
    )
    require(
        "ChunkTileSize = 16" in world_actor_h
        and "ChunkVisualComponents" in world_actor_h
        and "ResolveMetatileVisual(MetatileId)" in world_actor
        and "ComponentForMetatile(" in world_actor
        and "RenderPlane" in world_actor,
        "R5 renderer must partition tileset-aware render planes into chunks",
        errors,
    )
    require(
        "SetCollisionEnabled(ECollisionEnabled::NoCollision)" in world_actor,
        "R5 renderer must not duplicate authoritative gameplay collision",
        errors,
    )

    require(
        "RenderCatalog->ResolveMetatile(" in world_actor
        and "Metatile->RenderPlanes[PlaneIndex]" in world_actor
        and "Tile.SourceLayer != PlaneIndex" in world_actor,
        "R5 chunk geometry must be driven by packaged metatile descriptors",
        errors,
    )
    require(
        '#include "RemasterMetatileRenderMath.h"' in world_actor
        and "NumCustomDataFloats =" in world_actor
        and "metatile_render::CustomDataFloats" in world_actor
        and "metatile_render::custom_data_index(" in world_actor
        and "CustomField::TileId" in world_actor
        and "CustomField::Palette" in world_actor
        and "CustomField::HFlip" in world_actor
        and "CustomField::VFlip" in world_actor
        and "Tile.TileIdRaw" in world_actor
        and "Tile.Palette" in world_actor
        and "Tile.bHFlip" in world_actor
        and "Tile.bVFlip" in world_actor
        and "SetCustomDataValue(" in world_actor,
        "R5 render instances must preserve tile/palette/flip descriptor data through tested custom-data layout",
        errors,
    )
    require(
        'TEXT("bottom")' in world_actor
        and 'TEXT("middle")' in world_actor
        and 'TEXT("top")' in world_actor
        and "metatile_render::plane_height(" in world_actor,
        "R5 descriptor geometry must preserve deterministic render-plane ordering",
        errors,
    )

    require(
        "R5 packaged render catalog is not ready." in world_actor
        and "AddMissingDescriptorFallback(Visual, X, Y, Chunk)" in world_actor
        and 'TEXT("R7 visible descriptor fallback: %s in %s")' in world_actor
        and "Fallback->AddInstance" not in world_actor,
        "R7 must report descriptor gaps and retain a labeled visible plane; never guess replacement geometry",
        errors,
    )

    camera = (MODULE / "RemasterCameraRig.cpp").read_text(encoding="utf-8")
    environment = (MODULE / "RemasterEnvironmentController.cpp").read_text(encoding="utf-8")
    require("GetOverworldSnapshot(Snapshot)" in camera
        and "Snapshot.PlayerX, Snapshot.PlayerY" in camera
        and "LastMapRevision != Revision" in camera
        and "follow_alpha(FollowSpeed, DeltaSeconds)" in camera
        and "GetVelocity" not in camera,
        "R7 camera must follow read-only core coordinates and reset on every map rebuild", errors)
    require("Save->GetLocalRtcNow(Rtc)" in environment
        and "RuntimeWeatherId = Snapshot.Weather" in environment
        and "RealignRtcNow" not in environment
        and "LineTrace" not in camera
        and "RestoreCameraOcclusion" in world_actor
        and "can_admit(" in world_actor,
        "R7 environment must read core clock/weather and use render-only occlusion and tested budgets", errors)

    visual_style_h = (
        MODULE / "RemasterVisualStyle.h"
    ).read_text(encoding="utf-8")
    require(
        "FString Tileset;" in visual_style_h
        and "int32 LocalMetatileId = 0;" in visual_style_h
        and "int32 MetatileId = 0;" not in visual_style_h,
        "R5 visual rules must be keyed by tileset plus local metatile id",
        errors,
    )
    require(
        "MetatileId < 512u" in world_actor
        and "MetatileId - 512u" in world_actor
        and "LoadedMap.PrimaryTileset" in world_actor
        and "LoadedMap.SecondaryTileset" in world_actor,
        "R5 renderer must preserve Emerald primary/secondary metatile identity",
        errors,
    )

    require(
        "FRemasterGameplayMapChanged OnGameplayMapChanged" in world_gameplay_h
        and "OnGameplayMapChanged.Broadcast(" in world_gameplay,
        "R5 gameplay bridge must publish authoritative map changes",
        errors,
    )

    render_catalog_h = (
        MODULE / "RemasterRenderCatalogSubsystem.h"
    ).read_text(encoding="utf-8")
    render_catalog_cpp = (
        MODULE / "RemasterRenderCatalogSubsystem.cpp"
    ).read_text(encoding="utf-8")
    require(
        "URemasterRenderCatalogSubsystem" in render_catalog_h
        and 'TEXT("Generated")' in render_catalog_cpp
        and 'TEXT("Render")' in render_catalog_cpp
        and 'TEXT("manifest.json")' in render_catalog_cpp,
        "R5 render catalog must load Content/Generated/Render/manifest.json",
        errors,
    )
    require(
        "FindTileset(" in render_catalog_h
        and "LoadTileset(" in render_catalog_h
        and "ResolveMetatile(" in render_catalog_h
        and "DescriptorCache" in render_catalog_h,
        "R5 render catalog must provide exact lazy tileset/metatile resolution",
        errors,
    )
    require(
        "IsSafePackageRelative(" in render_catalog_cpp
        and "ResolvePackageFile(" in render_catalog_cpp
        and "FPaths::IsRelative" in render_catalog_cpp
        and 'Contains(TEXT("/../"))' in render_catalog_cpp,
        "R5 render catalog must reject package-path traversal",
        errors,
    )
    require(
        'Entries.Find(TilesetId)' in render_catalog_cpp
        and "LocalMetatileId" in render_catalog_cpp
        and "Metatiles.IsValidIndex(LocalMetatileId)" in render_catalog_cpp,
        "R5 render catalog must resolve exact tileset identity and local metatile id",
        errors,
    )

    render_catalog_h = (
        MODULE / "RemasterRenderCatalogSubsystem.h"
    ).read_text(encoding="utf-8")
    render_catalog_cpp = (
        MODULE / "RemasterRenderCatalogSubsystem.cpp"
    ).read_text(encoding="utf-8")
    require(
        'TEXT("Generated")' in render_catalog_cpp
        and 'TEXT("Render")' in render_catalog_cpp
        and 'TEXT("manifest.json")' in render_catalog_cpp
        and "vendor/vanillaplus" not in render_catalog_cpp,
        "R5 render catalog must load only the packaged Content/Generated/Render payload",
        errors,
    )
    require(
        "FindTileset(" in render_catalog_h
        and "LoadTileset(" in render_catalog_h
        and "ResolveMetatile(" in render_catalog_h
        and "Entries.Find(TilesetId)" in render_catalog_cpp,
        "R5 render catalog must resolve exact source tileset identities",
        errors,
    )
    require(
        "DescriptorCache.Find(TilesetId)" in render_catalog_cpp
        and "DescriptorCache.Add(TilesetId, Descriptor)" in render_catalog_cpp,
        "R5 render catalog must lazy-cache decoded tileset descriptors",
        errors,
    )
    require(
        "Entries.Num() == MetatileCount" not in render_catalog_cpp
        and "Entries.Num() == 8" in render_catalog_h
        and "Metatile.IsValid()" in render_catalog_cpp
        and "Reconstructed != Tile.RawU16" in render_catalog_cpp,
        "R5 render catalog must validate decoded metatile/tile integrity",
        errors,
    )
    require(
        "ResolvePackageFile(" in render_catalog_h
        and "IsSafePackageRelative(" in render_catalog_cpp
        and "FPaths::FileExists(Candidate)" in render_catalog_cpp,
        "R5 render catalog must reject package-path escapes and missing payload files",
        errors,
    )

    require(
        "TilesIndex8File" in render_catalog_h
        and "TilesIndex8Sha256" in render_catalog_h
        and "PaletteLutFile" in render_catalog_h
        and "PaletteLutSha256" in render_catalog_h
        and 'TEXT("tiles_index8_file")' in render_catalog_cpp
        and 'TEXT("palette_lut_file")' in render_catalog_cpp,
        "R5 render catalog must validate indexed tile and palette LUT payloads",
        errors,
    )

    render_resources_h = (
        MODULE / "RemasterRenderResourceSubsystem.h"
    ).read_text(encoding="utf-8")
    render_resources_cpp = (
        MODULE / "RemasterRenderResourceSubsystem.cpp"
    ).read_text(encoding="utf-8")
    require(
        "URemasterRenderResourceSubsystem" in render_resources_h
        and "LoadTilesetResources(" in render_resources_h
        and "ResourceCache" in render_resources_h,
        "R5 must expose a cached runtime render-resource subsystem",
        errors,
    )
    require(
        "PF_G8" in render_resources_cpp
        and "PF_B8G8R8A8" in render_resources_cpp
        and "TF_Nearest" in render_resources_cpp
        and "FFileHelper::LoadFileToArray(" in render_resources_cpp,
        "R5 runtime must upload packaged index/palette bytes as nearest-filter textures",
        errors,
    )
    require(
        "Entry->DescriptorSha256" in render_resources_cpp
        and "Entry->TilesIndex8Sha256" in render_resources_cpp
        and "Entry->PaletteLutSha256" in render_resources_cpp
        and "ResourceCache.Find(CacheKey)" in render_resources_cpp
        and "ResourceCache.Add(CacheKey, Resources)" in render_resources_cpp,
        "R5 render resources must be cached by tileset payload fingerprints",
        errors,
    )
    require(
        "ParseJascPalette" not in render_resources_cpp
        and "LoadFileToString" not in render_resources_cpp,
        "R5 runtime must consume prebuilt binary render payloads rather than decode source palettes per load",
        errors,
    )
    require(
        '#include "RemasterRenderResourceSubsystem.h"' in world_actor
        and "LoadTilesetResources(" in world_actor
        and "UMaterialInstanceDynamic::Create(" in world_actor
        and 'TEXT("R5_TileIndexTexture")' in world_actor
        and 'TEXT("R5_PaletteTexture")' in world_actor
        and 'TEXT("R5_TilesPerRow")' in world_actor,
        "R5 world renderer must bind cached index/palette resources into the material path",
        errors,
    )
    require(
        "TSoftObjectPtr<UMaterialInterface> MetatileMaterial;" in visual_style_h,
        "R5 visual style must expose the indexed metatile base-material contract",
        errors,
    )

    r5_pawn_h = (
        MODULE / "RemasterOverworldPawn.h"
    ).read_text(encoding="utf-8")
    r5_pawn_cpp = (
        MODULE / "RemasterOverworldPawn.cpp"
    ).read_text(encoding="utf-8")
    player_controller = (
        MODULE / "RemasterPlayerController.cpp"
    ).read_text(encoding="utf-8")
    game_mode = (
        MODULE / "R0GameMode.cpp"
    ).read_text(encoding="utf-8")

    require(
        "ApplyAuthoritativeStep(" in r5_pawn_h
        and "TileToWorldLocation(" in r5_pawn_cpp
        and "SetCollisionEnabled(ECollisionEnabled::NoCollision)" in r5_pawn_cpp,
        "R5 player pawn must be presentation-only and follow authoritative tile state",
        errors,
    )
    require(
        "Gameplay->StepPlayer(Direction, Result)" in player_controller
        and "OverworldPawn->ApplyAuthoritativeStep(Result)" in player_controller
        and "URemasterCoreSubsystem" not in player_controller,
        "R5 movement input must drive the authoritative R4 runtime, not the R0 prototype",
        errors,
    )
    require(
        "PlayerControllerClass = ARemasterPlayerController::StaticClass()" in game_mode
        and "DefaultPawnClass = ARemasterOverworldPawn::StaticClass()" in game_mode
        and "SpawnActor<ARemasterWorldActor>" in game_mode
        and "SpawnActor<ARemasterCameraRig>" in game_mode,
        "R5 default game mode must bootstrap a playable renderer/pawn/camera scene",
        errors,
    )
    require(
        "HUDClass = nullptr;" in game_mode
        and '#include "R0HUD.h"' not in game_mode,
        "R5 playable scene must not be covered by the legacy full-screen R0 HUD",
        errors,
    )

    # Continue path must preserve TEMP flags/vars exactly as Vanilla does.
    world_gameplay = (
        MODULE / "RemasterWorldGameplaySubsystem.cpp"
    ).read_text(encoding="utf-8")
    require(
        "LoadCurrentMapFromSave(false);" in world_gameplay,
        "saved-game startup must not clear temporary field state",
        errors,
    )
    require(
        "remaster_emerald_object_template_find_local_id" in world_gameplay
        and "remaster_emerald_object_templates_replace" in world_gameplay,
        "world gameplay bridge must synchronize saved object templates",
        errors,
    )

    world_gameplay_h = (
        MODULE / "RemasterWorldGameplaySubsystem.h"
    ).read_text(encoding="utf-8")
    require(
        "bool StepPlayer(" in world_gameplay_h
        and "FRemasterPlayerStepResult" in world_gameplay_h,
        "R4 Unreal gameplay bridge must expose the authoritative player-step API",
        errors,
    )
    require(
        '#include "remaster/emerald_overworld.h"' in world_gameplay
        and "remaster_emerald_overworld_step_action(" in world_gameplay
        and "remaster_emerald_overworld_continue_action(" in world_gameplay,
        "R4 Unreal gameplay bridge must use the shared overworld action runtime",
        errors,
    )
    require(
        "MapView.blocks = CurrentMap.RawBlocks.GetData();" in world_gameplay
        and "MapView.primary_attributes =" in world_gameplay
        and "MapView.secondary_attributes =" in world_gameplay,
        "R4 player movement must consume R3 decoded map data directly",
        errors,
    )
    require(
        "remaster_emerald_object_event_visible(" in world_gameplay
        and "RemasterEmeraldObjectCollider" in world_gameplay,
        "R4 player movement must feed visible world objects into collision",
        errors,
    )
    require(
        "ApplyResolvedWarp(Warp)" in world_gameplay
        and "ApplyResolvedConnection(Connection)" in world_gameplay,
        "R4 player-step adapter must reuse the production warp/connection paths",
        errors,
    )

    require(
        "RemasterEmeraldObjectRuntime" in world_gameplay
        and "RebuildRuntimeObjectState()" in world_gameplay
        and "remaster_emerald_object_runtime_build_colliders(" in world_gameplay,
        "R4 Unreal gameplay bridge must use portable runtime object state",
        errors,
    )
    require(
        "SetRuntimeObjectPosition(" in world_gameplay_h
        and "SetRuntimeObjectActive(" in world_gameplay_h
        and "SetRuntimeObjectPlayerCollisionExempt(" in world_gameplay_h,
        "R4 world/script host must expose transient runtime object mutations",
        errors,
    )

    require(
        "ContinuePlayerStepEvents(" in world_gameplay_h
        and "remaster_emerald_overworld_continue_action(" in world_gameplay,
        "R4 production movement must expose resumable shared-action processing",
        errors,
    )
    require(
        "remaster_emerald_process_step_events(" not in world_gameplay
        and "remaster_emerald_player_step(" not in world_gameplay,
        "R4 Unreal adapter must not duplicate the shared overworld action orchestration",
        errors,
    )

    world_data_h = (MODULE / "RemasterWorldData.h").read_text(encoding="utf-8")
    world_data_cpp = (MODULE / "RemasterWorldData.cpp").read_text(encoding="utf-8")
    world_catalog_cpp = (
        MODULE / "RemasterWorldCatalog.cpp"
    ).read_text(encoding="utf-8")

    for field in (
        "MusicId",
        "RegionMapSectionId",
        "BattleSceneId",
        "DirectionId",
        "ScriptId",
        "FingerprintSha256",
    ):
        require(
            field in world_data_h,
            f"R3 Unreal world data contract missing {field}",
            errors,
        )

    for json_field in (
        'TEXT("music_id")',
        'TEXT("region_map_section_id")',
        'TEXT("battle_scene_id")',
        'TEXT("direction_id")',
        'TEXT("script_id")',
        'TEXT("fingerprint_sha256")',
    ):
        require(
            json_field in world_data_cpp,
            f"R3 Unreal map parser missing {json_field}",
            errors,
        )

    require(
        'TryGetArrayField(TEXT("maps")' in world_catalog_cpp,
        "world catalog must continue loading maps from the R3 manifest",
        errors,
    )
    require(
        "layouts_file" not in world_catalog_cpp
        and "scripts_file" not in world_catalog_cpp
        and "encounters_file" not in world_catalog_cpp
        and "provenance_file" not in world_catalog_cpp,
        "world catalog must tolerate R3 auxiliary manifest fields "
        "without coupling gameplay loading to them",
        errors,
    )

    # R6 consumes R4 state and keeps asset readiness out of gameplay decisions.
    r6_visual = (MODULE / "RemasterCharacterVisualComponent.cpp").read_text()
    r6_world = (MODULE / "RemasterNpcPresentationWorld.cpp").read_text()
    r6_read = (MODULE / "RemasterCharacterPresentationRead.h").read_text()
    for source in (r6_visual, r6_world, r6_read):
        for mutation in ("remaster_emerald_flag_set(", "remaster_emerald_var_set(",
                         "remaster_emerald_object_runtime_set_", "StepPlayer(",
                         "ApplyResolvedWarp(", "SetRuntimeObjectPosition("):
            require(mutation not in source, "R6 presentation must not mutate gameplay: " + mutation, errors)
        require("pokemon-emerald-remastered-assets" not in source,
                "R6 runtime must not depend on the private vault", errors)
    for token in ("Entry.NormalizedSha256.Contains(Binding->NormalizedSha256)",
                  "bUnrealImportValidated", "RequestAsyncLoad(",
                  "WeakThis", "LoadGeneration.Accept(Generation)",
                  "ShowPlaceholder();", "ECollisionEnabled::NoCollision"):
        require(token in r6_visual, "R6 asset/fallback guard missing: " + token, errors)
    for token in ("GetObjectPresentationSnapshots(Snapshots)", "Instances.FindOrAdd(Snapshot.LocalId)",
                  "!Snapshot.bVisible", "InstanceMapId != MapId", "ClearInstances();",
                  "OnGameplayMapChanged.AddDynamic", "OnGameplayMapChanged.RemoveDynamic"):
        require(token in r6_world, "R6 authoritative instance lifecycle missing: " + token, errors)
    require("RemasterCharacterPresentation::ReadSnapshots(" in world_gameplay,
            "R6 bridge must use the natively tested read-only adapter", errors)
    require("remaster_emerald_object_runtime_get(" in r6_read
            and "remaster_emerald_object_event_visible(" in r6_read,
            "R6 snapshot must read portable runtime coordinates/visibility", errors)

    # R14 is a const R13 consumer; native timing tests complement these source guards.
    battle_read = (MODULE / "RemasterBattleRead.h").read_text()
    battle_stage = (MODULE / "RemasterBattleStage.cpp").read_text()
    battle_host = (MODULE / "RemasterBattlePresentationSubsystem.cpp").read_text()
    for source in (battle_read, battle_stage, battle_host):
        for operation in ("resolve_turn", "use_move", "calculate_damage", "choose_ai_action",
                          "switch", "replace_fainted", "throw_ball", "try_run", "clear_events", "finalize_trainer"):
            require(not re.search(r"remaster_emerald_battle_" + operation + r"\s*\(", source),
                    "R14 presentation called authoritative battle mutator: " + operation, errors)
    for token in ("const RemasterEmeraldBattleState&", "serial != lastSerial + 1", "droppedVisualCues += Pending()"):
        require(token in battle_read, "R14 bounded const sequencing guard missing: " + token, errors)
    for token in ("Generation != LoadGeneration[Slot]", "Key != ModelKeys[Slot]", "CreateWeakLambda",
                  "bUnrealImportValidated", "RequiredSpecialChannels.IsEmpty()", "ECollisionEnabled::NoCollision"):
        require(token in battle_stage, "R14 asset lifecycle guard missing: " + token, errors)
    require("Feed.Submit(" in battle_host and "Feed.Complete(" in battle_host,
            "R14 Unreal must use the natively tested battle feed", errors)

    # No production Unreal code should include Godot/SDL presentation APIs.
    forbidden = re.compile(r"\b(?:Godot|SDL3?|GDExtension)\b")
    for path in MODULE.glob("*.[ch]pp"):
        text = path.read_text(encoding="utf-8", errors="ignore")
        if forbidden.search(text):
            errors.append(f"legacy runtime reference in Unreal module: {path.name}")

    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        return 1

    print("Unreal source architecture validation passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
