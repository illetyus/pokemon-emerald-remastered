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
    core/src/emerald_map.c
    core/src/emerald_movement.c
    core/src/emerald_events.c
    core/src/emerald_transition.c
    core/src/emerald_script.c
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
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldMapEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldMovementEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldEventsEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldTransitionEmbed.cpp
    unreal/Source/PokemonEmeraldRemastered/RemasterEmeraldScriptEmbed.cpp
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
