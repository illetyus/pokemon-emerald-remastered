include(FetchContent)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
    URL_HASH SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68
)

FetchContent_MakeAvailable(SDL3)

add_executable(r0_sdl3
    main.c
)

target_link_libraries(r0_sdl3
    PRIVATE
        remaster_core
        SDL3::SDL3-static
)

if(MSVC)
    target_compile_options(r0_sdl3 PRIVATE /W4)
else()
    target_compile_options(r0_sdl3 PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
