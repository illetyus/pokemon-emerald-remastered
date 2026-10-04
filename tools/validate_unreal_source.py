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
    ]
    for filename in required_runtime_files:
        require((MODULE / filename).is_file(), f"missing runtime layer: {filename}", errors)

    # Guard against accidentally keying visuals by full map words. Visual
    # selection must use decoded metatile IDs while collision/elevation remain
    # gameplay/domain data.
    world_actor = (MODULE / "RemasterWorldActor.cpp").read_text(encoding="utf-8")
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
        and "remaster_emerald_player_step(" in world_gameplay,
        "R4 Unreal gameplay bridge must call the portable player-step core",
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
        "remaster_emerald_process_step_events(" in world_gameplay
        and "ContinuePlayerStepEvents(" in world_gameplay_h,
        "R4 production movement must expose resumable coord-before-warp processing",
        errors,
    )
    require(
        "nullptr,\n            0u,\n            static_cast<uint8>(Direction)" in world_gameplay,
        "R4 production player_step must not resolve warps before coord events",
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
