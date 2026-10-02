cmake_minimum_required(VERSION 3.16)
project(pokemon_emerald_remastered LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(remaster_core STATIC
    core/src/core.c
    core/src/platform.c
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
