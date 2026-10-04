#include "RemasterMetatileRenderMath.h"

#include <cstdio>

static int check(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "r5_metatile_render_test: %s\n", message);
        return 0;
    }
    return 1;
}

int main()
{
    using namespace remaster::metatile_render;

    if (!check(
            plane_height(PlaneOrder::Bottom, 1.5) == 0.0
            && plane_height(PlaneOrder::Middle, 1.5) == 1.5
            && plane_height(PlaneOrder::Top, 1.5) == 3.0,
            "render-plane Z ordering mismatch"))
        return 1;

    bool seen[CustomDataFloats] = {};
    for (int quadrant = 0; quadrant < QuadrantsPerPlane; ++quadrant) {
        for (int field = 0; field < FieldsPerQuadrant; ++field) {
            const int index = custom_data_index(
                quadrant,
                static_cast<CustomField>(field));
            if (!check(
                    index >= 0
                    && index < CustomDataFloats
                    && !seen[index],
                    "custom-data layout overlaps or escapes"))
                return 1;
            seen[index] = true;
        }
    }

    for (bool value : seen) {
        if (!check(value, "custom-data layout has a gap"))
            return 1;
    }

    if (!check(
            custom_data_index(-1, CustomField::TileId) == -1
            && custom_data_index(4, CustomField::TileId) == -1,
            "invalid quadrant guard mismatch"))
        return 1;

    std::puts("R5 metatile render math test passed.");
    return 0;
}
