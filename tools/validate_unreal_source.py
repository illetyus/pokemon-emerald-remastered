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

    expected_embeds = {
        "RemasterCoreEmbed.cpp": "../../../core/src/core.c",
        "RemasterPlatformEmbed.cpp": "../../../core/src/platform.c",
        "RemasterMechanicsEmbed.cpp": "../../../core/src/mechanics.c",
        "RemasterEmeraldSaveEmbed.cpp": "../../../core/src/emerald_save.c",
        "RemasterEmeraldRtcEmbed.cpp": "../../../core/src/emerald_rtc.c",
        "RemasterEmeraldStateEmbed.cpp": "../../../core/src/emerald_state.c",
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
        "RemasterOverworldPawn.cpp",
    ]
    for filename in required_runtime_files:
        require((MODULE / filename).is_file(), f"missing runtime layer: {filename}", errors)

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
