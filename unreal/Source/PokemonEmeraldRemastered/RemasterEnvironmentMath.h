#pragma once

#include <algorithm>
#include <cmath>
#include <string_view>

// Shared, engine-independent presentation policy. Never reads/writes collision,
// encounter, clock, weather-cycle or movement state.
namespace remaster::environment
{
constexpr int ChunkTiles = 16;
constexpr int MaxLod0Triangles = 6000;
constexpr int MaxLod1Triangles = 3000;
constexpr int MaxLod2Triangles = 1500;
constexpr int MaxMaterials = 2;
constexpr int MaxTextureDimension = 1024;
constexpr double MaxCullDistance = 4000.0;
constexpr int MaxComponentsPerChunk = 32;
constexpr int MaxInstancesPerChunk = 256;
constexpr int MaxTrianglesPerChunk = 60000;

inline bool can_admit(int components, int instances, int triangles,
                      int additional_triangles, bool new_component)
{
    return components >= 0 && components <= MaxComponentsPerChunk
        && (!new_component || components < MaxComponentsPerChunk)
        && instances >= 0 && instances < MaxInstancesPerChunk
        && triangles >= 0 && triangles <= MaxTrianglesPerChunk
        && additional_triangles > 0 && additional_triangles <= MaxLod0Triangles
        && additional_triangles <= MaxTrianglesPerChunk - triangles;
}

inline bool hash_valid(std::string_view hash)
{
    if (hash.size() != 64) return false;
    for (const char c : hash)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return false;
    return true;
}

inline bool budget_valid(int lod0, int lod1, int lod2, int materials,
                         int texture_dimension, double cull_distance)
{
    return lod0 > 0 && lod0 <= MaxLod0Triangles
        && lod1 > 0 && lod1 <= std::min(lod0, MaxLod1Triangles)
        && lod2 > 0 && lod2 <= std::min(lod1, MaxLod2Triangles)
        && materials > 0 && materials <= MaxMaterials
        && texture_dimension > 0 && texture_dimension <= MaxTextureDimension
        && std::isfinite(cull_distance) && cull_distance >= 1000.0
        && cull_distance <= MaxCullDistance;
}

inline double follow_alpha(double speed, double dt)
{
    if (!std::isfinite(speed) || !std::isfinite(dt) || speed <= 0 || dt <= 0)
        return 0.0;
    return -std::expm1(-speed * dt);
}

enum class Context { Outdoor, Indoor, Cave, Underwater };
inline Context context(int map_type)
{
    // Pinned Vanilla+ include/constants/map_types.h.
    if (map_type == 8 || map_type == 9) return Context::Indoor;
    if (map_type == 4) return Context::Cave;
    if (map_type == 5) return Context::Underwater;
    return Context::Outdoor;
}

struct Framing { double arm; double pitch; double fov; };
inline Framing framing(Context value)
{
    if (value == Context::Indoor) return {700.0, -65.0, 50.0};
    if (value == Context::Cave) return {800.0, -62.0, 50.0};
    if (value == Context::Underwater) return {850.0, -60.0, 50.0};
    return {900.0, -58.0, 50.0};
}

enum class Weather { Clear, Rain, Fog, Dust, Underwater, Shade, Drought, Unresolved };
inline Weather weather(int id)
{
    // Runtime weather IDs, never COORD_EVENT_WEATHER IDs.
    switch (id)
    {
    case 0: case 1: case 2: return Weather::Clear;
    case 3: case 5: case 13: return Weather::Rain;
    case 6: case 9: return Weather::Fog;
    case 7: case 8: return Weather::Dust;
    case 10: case 14: return Weather::Underwater;
    case 11: return Weather::Shade;
    case 12: return Weather::Drought;
    default: return Weather::Unresolved;
    }
}

inline double hour(double value)
{
    if (!std::isfinite(value)) return 12.0;
    const double wrapped = std::fmod(value, 24.0);
    return wrapped < 0.0 ? wrapped + 24.0 : wrapped;
}

struct Lighting { double solar; double fog; double ambient; };
inline Lighting lighting(double time, Context map_context, Weather value)
{
    const double solar = std::max(0.0, std::sin((hour(time) - 6.0) / 12.0
        * 3.14159265358979323846));
    if (map_context == Context::Indoor) return {0.0, 0.0, 0.8};
    if (map_context == Context::Cave) return {0.0, 0.008, 0.2};
    if (map_context == Context::Underwater) return {0.2, 0.02, 0.4};
    double fog = 0.012 - 0.009 * solar;
    if (value == Weather::Fog) fog = 0.04;
    if (value == Weather::Rain) fog = 0.012;
    if (value == Weather::Dust) fog = 0.025;
    return {solar, fog, 0.18 + 0.82 * solar};
}

struct Vec3 { double x; double y; double z; };
inline bool finite(Vec3 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

// Slab test on render bounds only, open endpoints prevent cutting objects at
// the player/camera. No physics trace and no authoritative collision changes.
inline bool occludes(Vec3 camera, Vec3 player, Vec3 minimum, Vec3 maximum)
{
    if (!finite(camera) || !finite(player) || !finite(minimum) || !finite(maximum))
        return false;
    const double origin[] = {camera.x, camera.y, camera.z};
    const double target[] = {player.x, player.y, player.z};
    const double low[] = {minimum.x, minimum.y, minimum.z};
    const double high[] = {maximum.x, maximum.y, maximum.z};
    double enter = 0.0, leave = 1.0;
    for (int axis = 0; axis < 3; ++axis)
    {
        if (low[axis] > high[axis]) return false;
        // Bounds containing either endpoint are deliberately retained.
        const double delta = target[axis] - origin[axis];
        if (std::abs(delta) < 1e-9)
        {
            if (origin[axis] < low[axis] || origin[axis] > high[axis]) return false;
            continue;
        }
        double a = (low[axis] - origin[axis]) / delta;
        double b = (high[axis] - origin[axis]) / delta;
        if (a > b) std::swap(a, b);
        enter = std::max(enter, a);
        leave = std::min(leave, b);
        if (enter > leave) return false;
    }
    return enter > 0.01 && leave < 0.99;
}
}
