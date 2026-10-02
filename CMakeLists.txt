cmake_minimum_required(VERSION 3.16)
project(pokemon_emerald_remastered LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(remaster_core STATIC
    core/src/core.c
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
