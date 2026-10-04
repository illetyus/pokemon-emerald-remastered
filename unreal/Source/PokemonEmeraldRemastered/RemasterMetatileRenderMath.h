#ifndef REMASTER_METATILE_RENDER_MATH_H
#define REMASTER_METATILE_RENDER_MATH_H

#include <cstdint>

namespace remaster::metatile_render
{
enum class PlaneOrder : int32_t
{
    Invalid = -1,
    Bottom = 0,
    Middle = 1,
    Top = 2
};

enum class CustomField : int32_t
{
    TileId = 0,
    Palette = 1,
    HFlip = 2,
    VFlip = 3
};

inline constexpr int32_t QuadrantsPerPlane = 4;
inline constexpr int32_t FieldsPerQuadrant = 4;
inline constexpr int32_t CustomDataFloats =
    QuadrantsPerPlane * FieldsPerQuadrant;

inline double plane_height(
    PlaneOrder plane,
    double spacing)
{
    if (!(spacing >= 0.0))
        return 0.0;

    switch (plane) {
    case PlaneOrder::Bottom:
        return 0.0;
    case PlaneOrder::Middle:
        return spacing;
    case PlaneOrder::Top:
        return spacing * 2.0;
    default:
        return 0.0;
    }
}

inline int32_t custom_data_index(
    int32_t quadrant,
    CustomField field)
{
    if (quadrant < 0 || quadrant >= QuadrantsPerPlane)
        return -1;

    const int32_t field_index =
        static_cast<int32_t>(field);
    if (field_index < 0
        || field_index >= FieldsPerQuadrant)
    {
        return -1;
    }

    return quadrant * FieldsPerQuadrant + field_index;
}
}

#endif
