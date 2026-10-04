#ifndef REMASTER_WORLD_GRID_MATH_H
#define REMASTER_WORLD_GRID_MATH_H

#include <cmath>
#include <cstdint>

namespace remaster::world_grid
{
struct ChunkCoord
{
    int32_t x = 0;
    int32_t y = 0;

    bool operator==(const ChunkCoord& other) const
    {
        return x == other.x && y == other.y;
    }
};

inline int32_t floor_div(int32_t value, int32_t divisor)
{
    if (divisor <= 0)
        return 0;

    if (value >= 0)
        return value / divisor;

    return -static_cast<int32_t>(
        (static_cast<int64_t>(-value) + divisor - 1) / divisor);
}

inline ChunkCoord chunk_for_tile(
    int32_t tile_x,
    int32_t tile_y,
    int32_t chunk_tile_size)
{
    return {
        floor_div(tile_x, chunk_tile_size),
        floor_div(tile_y, chunk_tile_size)
    };
}

inline double tile_axis_to_local(
    int32_t tile,
    double tile_world_size)
{
    return static_cast<double>(tile) * tile_world_size;
}

inline int32_t local_axis_to_tile(
    double local,
    double tile_world_size)
{
    if (!(tile_world_size > 0.0))
        return 0;

    return static_cast<int32_t>(
        std::llround(local / tile_world_size));
}
}

#endif
