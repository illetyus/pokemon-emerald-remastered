#include "RemasterWorldGridMath.h"

#include <cmath>
#include <cstdio>

static int check(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "r5_world_grid_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main()
{
    using namespace remaster::world_grid;

    if (!check(
            chunk_for_tile(0, 0, 16) == ChunkCoord{0, 0}
            && chunk_for_tile(15, 15, 16) == ChunkCoord{0, 0}
            && chunk_for_tile(16, 0, 16) == ChunkCoord{1, 0}
            && chunk_for_tile(31, 32, 16) == ChunkCoord{1, 2},
            "positive tile chunk partition mismatch"))
        return 1;

    if (!check(
            chunk_for_tile(-1, -1, 16) == ChunkCoord{-1, -1}
            && chunk_for_tile(-16, -16, 16) == ChunkCoord{-1, -1}
            && chunk_for_tile(-17, -17, 16) == ChunkCoord{-2, -2},
            "negative tile floor-division mismatch"))
        return 1;

    constexpr double tile_size = 100.0;
    for (int32_t tile = -64; tile <= 64; ++tile) {
        const double local = tile_axis_to_local(tile, tile_size);
        if (!check(
                local_axis_to_tile(local, tile_size) == tile,
                "tile/local round trip mismatch"))
            return 1;
    }

    if (!check(
            local_axis_to_tile(149.0, tile_size) == 1
            && local_axis_to_tile(151.0, tile_size) == 2
            && local_axis_to_tile(-149.0, tile_size) == -1
            && local_axis_to_tile(-151.0, tile_size) == -2,
            "nearest-tile rounding mismatch"))
        return 1;

    if (!check(
            local_axis_to_tile(123.0, 0.0) == 0
            && local_axis_to_tile(123.0, -1.0) == 0,
            "invalid tile size guard mismatch"))
        return 1;

    std::puts("R5 world-grid math test passed.");
    return 0;
}
