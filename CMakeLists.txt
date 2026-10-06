cmake_minimum_required(VERSION 3.16)
project(pokemon_emerald_remastered LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(remaster_core STATIC
    core/src/core.c
    core/src/platform.c
    core/src/mechanics.c
    core/src/emerald_save.c
    core/src/emerald_rtc.c
    core/src/emerald_state.c
    core/src/emerald_quest.c
    core/src/emerald_pokemon.c
    core/src/emerald_items.c
    core/src/emerald_qol.c
    core/src/emerald_encounter.c
    core/src/emerald_battle.c
    core/src/emerald_map.c
    core/src/emerald_movement.c
    core/src/emerald_events.c
    core/src/emerald_transition.c
    core/src/emerald_script.c
    core/src/emerald_script_runtime.c
    core/src/emerald_script_checkpoint.c
    core/src/emerald_object_state.c
)

set_target_properties(remaster_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(remaster_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
)

if(MSVC)
    target_compile_options(remaster_core PRIVATE /W4)
else()
    target_compile_options(remaster_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

enable_testing()

add_executable(core_smoke_test
    tests/core_smoke.c
)

target_link_libraries(core_smoke_test PRIVATE remaster_core)

add_test(NAME core_smoke COMMAND core_smoke_test)


option(REMASTER_BUILD_SDL3 "Build the SDL3 native R0 presentation prototype" OFF)

if(REMASTER_BUILD_SDL3)
    add_subdirectory(experiments/sdl3-native)
endif()


option(REMASTER_BUILD_GODOT_BRIDGE "Build the Godot GDExtension R0 bridge" OFF)

if(REMASTER_BUILD_GODOT_BRIDGE)
    enable_language(CXX)
    add_subdirectory(experiments/godot-bridge)
endif()


# Unreal integration preflight: compile the portable C implementation inside
# a C++ translation unit exactly as the Unreal runtime module embeds it.
enable_language(CXX)

add_executable(unreal_cpp_embed_smoke
    tests/unreal_cpp_embed_smoke.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterCoreEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterPlatformEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterMechanicsEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldSaveEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldRtcEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldStateEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldQuestEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldPokemonEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldItemsEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldQolEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldEncounterEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldBattleEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldMapEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldMovementEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldEventsEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldTransitionEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldScriptEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldScriptRuntimeEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldObjectStateEmbed.cpp
)

target_include_directories(unreal_cpp_embed_smoke
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
)

target_compile_features(unreal_cpp_embed_smoke PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(unreal_cpp_embed_smoke PRIVATE /W4)
else()
    target_compile_options(unreal_cpp_embed_smoke PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME unreal_cpp_embed COMMAND unreal_cpp_embed_smoke)


add_executable(r5_metatile_render_test
    tests/r5_metatile_render_test.cpp
)

target_include_directories(r5_metatile_render_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/unreal/Source/PokemonEmeraldRemastered
)

target_compile_features(r5_metatile_render_test PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(r5_metatile_render_test PRIVATE /W4)
else()
    target_compile_options(r5_metatile_render_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME r5_metatile_render_math COMMAND r5_metatile_render_test)


add_executable(r5_world_grid_test
    tests/r5_world_grid_test.cpp
)

target_include_directories(r5_world_grid_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/unreal/Source/PokemonEmeraldRemastered
)

target_compile_features(r5_world_grid_test PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(r5_world_grid_test PRIVATE /W4)
else()
    target_compile_options(r5_world_grid_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME r5_world_grid_math COMMAND r5_world_grid_test)


add_executable(core_replay_test
    tests/core_replay.c
)

target_link_libraries(core_replay_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(core_replay_test PRIVATE /W4)
else()
    target_compile_options(core_replay_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME core_replay COMMAND core_replay_test)


add_executable(platform_mechanics_contract_test
    tests/platform_mechanics_contract.c
)

target_link_libraries(platform_mechanics_contract_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(platform_mechanics_contract_test PRIVATE /W4)
else()
    target_compile_options(platform_mechanics_contract_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME platform_mechanics_contract COMMAND platform_mechanics_contract_test)


add_executable(emerald_save_test
    tests/emerald_save_test.c
)

target_link_libraries(emerald_save_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_save_test PRIVATE /W4)
else()
    target_compile_options(emerald_save_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_save_compatibility COMMAND emerald_save_test)

add_executable(emerald_save_validation_test
    tests/emerald_save_validation_test.c
)

target_link_libraries(emerald_save_validation_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_save_validation_test PRIVATE /W4)
else()
    target_compile_options(emerald_save_validation_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME r17_save_sector_validation COMMAND emerald_save_validation_test)


add_executable(emerald_rtc_test
    tests/emerald_rtc_test.c
)

target_link_libraries(emerald_rtc_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_rtc_test PRIVATE /W4)
else()
    target_compile_options(emerald_rtc_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_rtc_compatibility COMMAND emerald_rtc_test)


add_executable(emerald_save_platform_test
    tests/emerald_save_platform_test.c
)

target_link_libraries(emerald_save_platform_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_save_platform_test PRIVATE /W4)
else()
    target_compile_options(emerald_save_platform_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_save_platform_io COMMAND emerald_save_platform_test)


add_executable(emerald_state_test
    tests/emerald_state_test.c
)

target_link_libraries(emerald_state_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_state_test PRIVATE /W4)
else()
    target_compile_options(emerald_state_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_overworld_state COMMAND emerald_state_test)


add_executable(emerald_map_test
    tests/emerald_map_test.c
)

target_link_libraries(emerald_map_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_map_test PRIVATE /W4)
else()
    target_compile_options(emerald_map_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_map_grid COMMAND emerald_map_test)


add_executable(emerald_movement_test
    tests/emerald_movement_test.c
)

target_link_libraries(emerald_movement_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_movement_test PRIVATE /W4)
else()
    target_compile_options(emerald_movement_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_movement_collision COMMAND emerald_movement_test)



add_executable(emerald_overworld_step_test
    tests/emerald_overworld_step_test.c
)

target_link_libraries(emerald_overworld_step_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_overworld_step_test PRIVATE /W4)
else()
    target_compile_options(emerald_overworld_step_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_overworld_player_step COMMAND emerald_overworld_step_test)

add_executable(emerald_step_events_test
    tests/emerald_step_events_test.c
)

target_link_libraries(emerald_step_events_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_step_events_test PRIVATE /W4)
else()
    target_compile_options(emerald_step_events_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_step_event_ordering COMMAND emerald_step_events_test)


add_executable(emerald_events_test
    tests/emerald_events_test.c
)

target_link_libraries(emerald_events_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_events_test PRIVATE /W4)
else()
    target_compile_options(emerald_events_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_map_events COMMAND emerald_events_test)


add_executable(emerald_transition_test
    tests/emerald_transition_test.c
)

target_link_libraries(emerald_transition_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_transition_test PRIVATE /W4)
else()
    target_compile_options(emerald_transition_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_warp_transition COMMAND emerald_transition_test)


add_executable(emerald_script_test
    tests/emerald_script_test.c
)

target_link_libraries(emerald_script_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_script_test PRIVATE /W4)
else()
    target_compile_options(emerald_script_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_script_vm COMMAND emerald_script_test)

add_executable(emerald_script_runtime_test
    tests/emerald_script_runtime_test.c
)

target_link_libraries(emerald_script_runtime_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_script_runtime_test PRIVATE /W4)
else()
    target_compile_options(emerald_script_runtime_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_script_runtime COMMAND emerald_script_runtime_test)

add_executable(emerald_script_trainer_battle_test
    tests/emerald_script_trainer_battle_test.c
)
target_link_libraries(emerald_script_trainer_battle_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_script_trainer_battle_test PRIVATE /W4)
else()
    target_compile_options(emerald_script_trainer_battle_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r13_trainer_script_bridge COMMAND emerald_script_trainer_battle_test)

add_executable(emerald_script_checkpoint_test
    tests/emerald_script_checkpoint_test.c
)

target_link_libraries(emerald_script_checkpoint_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_script_checkpoint_test PRIVATE /W4)
else()
    target_compile_options(emerald_script_checkpoint_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_script_checkpoint COMMAND emerald_script_checkpoint_test)

add_executable(emerald_script_object_test
    tests/emerald_script_object_test.c
)

target_link_libraries(emerald_script_object_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_script_object_test PRIVATE /W4)
else()
    target_compile_options(emerald_script_object_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_script_object COMMAND emerald_script_object_test)


add_executable(r2_opening_commands_test
    tests/r2_opening_commands_test.c
)

target_link_libraries(r2_opening_commands_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(r2_opening_commands_test PRIVATE /W4)
else()
    target_compile_options(r2_opening_commands_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME r2_opening_commands COMMAND r2_opening_commands_test)


find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(R11_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/r11")
set(R11_DOMAIN_CATALOG "${R11_GENERATED_DIR}/emerald_domain_catalog.inc")
set(R11_DOMAIN_REPORT "${R11_GENERATED_DIR}/domain_catalog_report.json")
file(MAKE_DIRECTORY "${R11_GENERATED_DIR}")

execute_process(
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/build_r11_domain_catalog.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/vanillaplus"
        "${R11_DOMAIN_CATALOG}"
        "${R11_DOMAIN_REPORT}"
    RESULT_VARIABLE R11_DOMAIN_GENERATE_RESULT
)
if(NOT R11_DOMAIN_GENERATE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate R11 Pokémon domain catalog")
endif()

target_include_directories(remaster_core
    PRIVATE
        "${R11_GENERATED_DIR}"
)

target_include_directories(unreal_cpp_embed_smoke
    PRIVATE
        "${R11_GENERATED_DIR}"
)


set(R12_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/r12")
set(R12_ENCOUNTER_CATALOG "${R12_GENERATED_DIR}/emerald_encounter_catalog.inc")
set(R12_ENCOUNTER_REPORT "${R12_GENERATED_DIR}/encounter_catalog_report.json")
file(MAKE_DIRECTORY "${R12_GENERATED_DIR}")

execute_process(
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/build_r12_encounter_catalog.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/vanillaplus"
        "${R12_ENCOUNTER_CATALOG}"
        "${R12_ENCOUNTER_REPORT}"
    RESULT_VARIABLE R12_ENCOUNTER_GENERATE_RESULT
)
if(NOT R12_ENCOUNTER_GENERATE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate R12 encounter catalog")
endif()

target_include_directories(remaster_core
    PRIVATE
        "${R12_GENERATED_DIR}"
)

target_include_directories(unreal_cpp_embed_smoke
    PRIVATE
        "${R12_GENERATED_DIR}"
)


set(R13_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/r13")
set(R13_TRAINER_CATALOG "${R13_GENERATED_DIR}/emerald_trainer_catalog.inc")
set(R13_TRAINER_REPORT "${R13_GENERATED_DIR}/trainer_catalog_report.json")
file(MAKE_DIRECTORY "${R13_GENERATED_DIR}")

execute_process(
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/build_r13_trainer_catalog.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/vanillaplus"
        "${R13_TRAINER_CATALOG}"
        "${R13_TRAINER_REPORT}"
    RESULT_VARIABLE R13_TRAINER_GENERATE_RESULT
)
if(NOT R13_TRAINER_GENERATE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate R13 trainer catalog")
endif()

target_include_directories(remaster_core
    PRIVATE
        "${R13_GENERATED_DIR}"
)

target_include_directories(unreal_cpp_embed_smoke
    PRIVATE
        "${R13_GENERATED_DIR}"
)
set(R2_LITTLEROOT_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(R2_LITTLEROOT_GENERATED_C "${R2_LITTLEROOT_GENERATED_DIR}/r2_littleroot_generated.c")
file(MAKE_DIRECTORY "${R2_LITTLEROOT_GENERATED_DIR}")

execute_process(
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/build_r2_littleroot_fixture.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/r2/vanillaplus_source"
        "${R2_LITTLEROOT_GENERATED_C}"
    RESULT_VARIABLE R2_LITTLEROOT_GENERATE_RESULT
)
if(NOT R2_LITTLEROOT_GENERATE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate real R2 Littleroot fixture")
endif()

add_executable(r2_littleroot_integration_test
    tests/r2_littleroot_integration_test.c
    "${R2_LITTLEROOT_GENERATED_C}"
)

target_link_libraries(r2_littleroot_integration_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(r2_littleroot_integration_test PRIVATE /W4)
else()
    target_compile_options(r2_littleroot_integration_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME r2_littleroot_integration COMMAND r2_littleroot_integration_test)


set(R4_LITTLEROOT_GENERATED_C "${R2_LITTLEROOT_GENERATED_DIR}/r4_littleroot_world_generated.c")

execute_process(
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/build_r4_littleroot_world_fixture.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/vanillaplus"
        "${R4_LITTLEROOT_GENERATED_C}"
    RESULT_VARIABLE R4_LITTLEROOT_GENERATE_RESULT
)
if(NOT R4_LITTLEROOT_GENERATE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate real R4 Littleroot world fixture")
endif()

add_executable(r4_littleroot_overworld_test
    tests/r4_littleroot_overworld_test.c
    "${R4_LITTLEROOT_GENERATED_C}"
)

target_link_libraries(r4_littleroot_overworld_test PRIVATE remaster_core)

target_include_directories(r4_littleroot_overworld_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

if(MSVC)
    target_compile_options(r4_littleroot_overworld_test PRIVATE /W4)
else()
    target_compile_options(r4_littleroot_overworld_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME r4_littleroot_overworld COMMAND r4_littleroot_overworld_test)


add_executable(r1_vanillaplus_integration_test
    tests/r1_vanillaplus_integration_test.c
)

target_link_libraries(r1_vanillaplus_integration_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(r1_vanillaplus_integration_test PRIVATE /W4)
else()
    target_compile_options(r1_vanillaplus_integration_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME r1_vanillaplus_integration COMMAND r1_vanillaplus_integration_test)


add_executable(emerald_save_inspect
    tools/emerald_save_inspect.c
)

target_link_libraries(emerald_save_inspect PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_save_inspect PRIVATE /W4)
else()
    target_compile_options(emerald_save_inspect PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()


add_executable(emerald_object_runtime_test
    tests/emerald_object_runtime_test.c
)

target_link_libraries(emerald_object_runtime_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_object_runtime_test PRIVATE /W4)
else()
    target_compile_options(emerald_object_runtime_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_runtime_object_collision COMMAND emerald_object_runtime_test)


add_executable(emerald_object_state_test
    tests/emerald_object_state_test.c
)

target_link_libraries(emerald_object_state_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_object_state_test PRIVATE /W4)
else()
    target_compile_options(emerald_object_state_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME emerald_object_save_state COMMAND emerald_object_state_test)


add_executable(emerald_quest_test
    tests/emerald_quest_test.c
)

target_link_libraries(emerald_quest_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_quest_test PRIVATE /W4)
else()
    target_compile_options(emerald_quest_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME r10_quest_progression COMMAND emerald_quest_test)


add_executable(emerald_quest_save_test
    tests/emerald_quest_save_test.c
)

target_link_libraries(emerald_quest_save_test PRIVATE remaster_core)

if(MSVC)
    target_compile_options(emerald_quest_save_test PRIVATE /W4)
else()
    target_compile_options(emerald_quest_save_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME r10_quest_save_roundtrip COMMAND emerald_quest_save_test)


add_test(
    NAME r10_quest_catalog_audit
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_r10_quest_catalog.py"
)


add_executable(emerald_pokemon_codec_test
    tests/emerald_pokemon_codec_test.c
)
target_link_libraries(emerald_pokemon_codec_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_pokemon_codec_test PRIVATE /W4)
else()
    target_compile_options(emerald_pokemon_codec_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r11_box_pokemon_codec COMMAND emerald_pokemon_codec_test)


add_executable(emerald_party_storage_test
    tests/emerald_party_storage_test.c
)
target_link_libraries(emerald_party_storage_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_party_storage_test PRIVATE /W4)
else()
    target_compile_options(emerald_party_storage_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r11_party_storage COMMAND emerald_party_storage_test)


add_executable(emerald_bag_test
    tests/emerald_bag_test.c
)
target_link_libraries(emerald_bag_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_bag_test PRIVATE /W4)
else()
    target_compile_options(emerald_bag_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r11_bag COMMAND emerald_bag_test)


add_executable(emerald_domain_catalog_test
    tests/emerald_domain_catalog_test.c
)
target_link_libraries(emerald_domain_catalog_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_domain_catalog_test PRIVATE /W4)
else()
    target_compile_options(emerald_domain_catalog_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r11_domain_catalog COMMAND emerald_domain_catalog_test)

add_executable(emerald_qol_test
    tests/emerald_qol_test.c
)
target_link_libraries(emerald_qol_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_qol_test PRIVATE /W4)
else()
    target_compile_options(emerald_qol_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r16_qol_behavior COMMAND emerald_qol_test)

add_test(
    NAME r16_qol_source_audit
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_r16_qol_source.py"
)


add_test(
    NAME r11_domain_catalog_audit
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_r11_domain_catalog.py"
)


add_executable(emerald_encounter_rng_test
    tests/emerald_encounter_rng_test.c
)
target_link_libraries(emerald_encounter_rng_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_encounter_rng_test PRIVATE /W4)
else()
    target_compile_options(emerald_encounter_rng_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r12_encounter_rng COMMAND emerald_encounter_rng_test)


add_executable(emerald_encounter_rules_test
    tests/emerald_encounter_rules_test.c
)
target_link_libraries(emerald_encounter_rules_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_encounter_rules_test PRIVATE /W4)
else()
    target_compile_options(emerald_encounter_rules_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r12_encounter_rules COMMAND emerald_encounter_rules_test)


add_executable(emerald_encounter_ecosystem_test
    tests/emerald_encounter_ecosystem_test.c
)
target_link_libraries(emerald_encounter_ecosystem_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_encounter_ecosystem_test PRIVATE /W4)
else()
    target_compile_options(emerald_encounter_ecosystem_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r12_encounter_ecosystem COMMAND emerald_encounter_ecosystem_test)


add_executable(emerald_encounter_special_test
    tests/emerald_encounter_special_test.c
)
target_link_libraries(emerald_encounter_special_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_encounter_special_test PRIVATE /W4)
else()
    target_compile_options(emerald_encounter_special_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r12_encounter_special COMMAND emerald_encounter_special_test)


add_executable(emerald_encounter_replay_test
    tests/emerald_encounter_replay_test.c
)
target_link_libraries(emerald_encounter_replay_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_encounter_replay_test PRIVATE /W4)
else()
    target_compile_options(emerald_encounter_replay_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r12_encounter_replay COMMAND emerald_encounter_replay_test)


add_test(
    NAME r12_encounter_catalog_audit
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_r12_encounter_catalog.py"
)


add_executable(emerald_battle_core_test
    tests/emerald_battle_core_test.c
)
target_link_libraries(emerald_battle_core_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_battle_core_test PRIVATE /W4)
else()
    target_compile_options(emerald_battle_core_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r13_battle_core COMMAND emerald_battle_core_test)


add_executable(emerald_battle_trainer_test
    tests/emerald_battle_trainer_test.c
)
target_link_libraries(emerald_battle_trainer_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_battle_trainer_test PRIVATE /W4)
else()
    target_compile_options(emerald_battle_trainer_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r13_battle_trainer_contract COMMAND emerald_battle_trainer_test)


add_executable(emerald_battle_ai_test
    tests/emerald_battle_ai_test.c
)
target_link_libraries(emerald_battle_ai_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_battle_ai_test PRIVATE /W4)
else()
    target_compile_options(emerald_battle_ai_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r13_battle_ai COMMAND emerald_battle_ai_test)


add_executable(emerald_battle_replacement_test
    tests/emerald_battle_replacement_test.c
)
target_link_libraries(emerald_battle_replacement_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_battle_replacement_test PRIVATE /W4)
else()
    target_compile_options(emerald_battle_replacement_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r13_battle_replacement COMMAND emerald_battle_replacement_test)


add_executable(emerald_battle_progression_test
    tests/emerald_battle_progression_test.c
)
target_link_libraries(emerald_battle_progression_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_battle_progression_test PRIVATE /W4)
else()
    target_compile_options(emerald_battle_progression_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r13_battle_progression COMMAND emerald_battle_progression_test)


add_executable(emerald_battle_replay_test
    tests/emerald_battle_replay_test.c
)
target_link_libraries(emerald_battle_replay_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_battle_replay_test PRIVATE /W4)
else()
    target_compile_options(emerald_battle_replay_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r13_battle_replay COMMAND emerald_battle_replay_test)


add_test(
    NAME r13_trainer_catalog_audit
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_r13_trainer_catalog.py"
)


add_executable(emerald_battle_integration_test
    tests/emerald_battle_integration_test.c
)
target_link_libraries(emerald_battle_integration_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_battle_integration_test PRIVATE /W4)
else()
    target_compile_options(emerald_battle_integration_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r13_encounter_battle_integration COMMAND emerald_battle_integration_test)


add_executable(emerald_battle_effects_test
    tests/emerald_battle_effects_test.c
)
target_link_libraries(emerald_battle_effects_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_battle_effects_test PRIVATE /W4)
else()
    target_compile_options(emerald_battle_effects_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r13_battle_effects COMMAND emerald_battle_effects_test)


add_executable(emerald_battle_move_effects_test
    tests/emerald_battle_move_effects_test.c
)
target_link_libraries(emerald_battle_move_effects_test PRIVATE remaster_core)
if(MSVC)
    target_compile_options(emerald_battle_move_effects_test PRIVATE /W4)
else()
    target_compile_options(emerald_battle_move_effects_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME r13_battle_move_effects COMMAND emerald_battle_move_effects_test)
