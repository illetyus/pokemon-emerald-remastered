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
