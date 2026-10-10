# Production dependencies and historical reference pins

R20 source-preparation inventory. The production module does not fetch SDL,
Godot, godot-cpp or the multiplatform reference. Their R0 pins below are
historical and their build options are OFF in ordinary production CMake.

## Portable/source preparation

- Complete Git checkout, including the unchanged vendored Vanilla+ snapshot.
  Source pin: 70db90c9077aed1272e746fc2537d9f12b95a91c; accepted vendored tree:
  5a551f1f9e40184278c57dfb8d25f68a0a1c99dc. external/sources.lock.json and
  vendor/VANILLAPLUS_SOURCE.md bind this input; R3 verifies the actual Git tree.
- C99 compiler and C++17 compiler; Windows native persistence tests use MSVC.
- CMake >=3.16 for configuration; the complete R19 suite uses CTest JSON discovery
  and --output-junit, requiring CMake/CTest >=3.21. The configure minimum alone
  does not promise full-suite compatibility with older CTest.
- Python >=3.10; source generators/test tooling use the standard library.
- FFmpeg and ffprobe must be on PATH for the required audio transform tests;
  CI installs both and sets REMASTER_R15_REQUIRE_FFMPEG=1. Record actual versions
  when reproducing audio; historical candidate hashes remain explicit and a
  different codec/toolchain is not silently treated as the same private pack.
- No private ROM/model/texture/audio path or account is required for the public
  source/portable acceptance suite. Private normalization/listening/import work
  has explicit optional inputs and evidence in the phase documents.

No third-party Python runtime package or automatically downloaded gameplay
dependency is required by the ordinary portable/source suite.

## Unreal production runtime

The required engine build is exactly UE 5.8.3, verified from the real installation
at R18. The uproject's 5.8 association identifies the engine family, not patch
success. Android SDK/NDK/JDK/UBT requirements must be read from that exact engine
installation and matched at R18; version numbers are not guessed here.

Retained public modules:
Core, CoreUObject, Engine (types/subsystems/actors), InputCore (physical keys),
Json (source package parsing), UMG (widgets), EnhancedInput (action bindings),
Niagara (presentation feedback), DeveloperSettings (local presentation settings).
Retained private modules: ApplicationCore (lifecycle/platform), Slate and
SlateCore (native widget/input support).

R20 removed JsonUtilities and Projects because the complete module has no
consumer for their APIs/headers. Json parsing uses JsonReader/JsonSerializer
from Json, and module registration uses Core's ModuleManager. This is a source
dependency cleanup; real UBT/UHT/link acceptance remains R18.

Private normalized/imported visual/audio packages are optional presentation
bindings with explicit fallbacks. Their original source, normalization recipes,
hashes, import readiness and quality limits are documented in R6/R7/R14/R15.
Keep all private payloads and APK/engine outputs outside public source tracking.

## Historical R0 evidence (preserved pins)

R0 intentionally pins external toolchain dependencies. Architecture comparisons are not useful if each candidate silently moves to a different engine or API version.

## Godot

- Version: 4.7.2-stable
- Release date: 2026-08-18
- Linux x86_64 archive:
  `Godot_v4.7.2-stable_linux.x86_64.zip`
- SHA-256:
  `cadd3204e728a35d3f13adb7fd0d7902636b79f6b95c40c265eb73b6c35329e4`

R0 does not use Godot 4.8 development snapshots.

## godot-cpp

- Version: 10.0.0-stable
- Pinned commit:
  `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`
- Target API version: Godot 4.7

The bridge never tracks `master`.

## SDL

- Version: 3.4.16
- Source archive:
  `SDL3-3.4.16.tar.gz`
- SHA-256:
  `7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68`

## R0 state contract

Canonical scenario:
`shared/scenarios/r0_walk_event_encounter.json`

Canonical final C-state hash:
`0x642df4e66c5448a0`

Dependencies may only be changed during R0 with an explicit commit explaining why. Once R0 selects a production architecture, dependency updates move through normal compatibility testing.

