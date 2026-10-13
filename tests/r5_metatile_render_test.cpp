#include "RemasterMetatileRenderMath.h"

#include <cstdio>
#include <vector>
#include "RemasterIndexedMapMath.h"

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

    using namespace RemasterIndexedMap;
    std::vector<std::uint8_t> first(512*64),second(512*64),atlas(AtlasWidth*AtlasHeight,0);
    for(int tile=0;tile<512;++tile)for(int y=0;y<8;++y)for(int x=0;x<8;++x) {
        first[(tile/8*8+y)*64+tile%8*8+x]=static_cast<std::uint8_t>((tile+x+3*y)&15);
        second[(tile/32*8+y)*256+tile%32*8+x]=static_cast<std::uint8_t>((5*tile+3*x+y)&15);
    }
    if(!check(CopyIndex(first.data(),first.size(),64,512,0,atlas.data(),atlas.size())
              &&CopyIndex(second.data(),second.size(),256,128,512,atlas.data(),atlas.size()),"different source atlas widths compose into source VRAM partitions"))return 1;
    for(int tile=0;tile<1024;++tile)for(int y=0;y<8;++y)for(int x=0;x<8;++x) {
        const auto expected=(tile<512?tile+x+3*y:5*(tile-512)+3*x+y)&15;
        if(!check(atlas[(tile/16*8+y)*128+tile%16*8+x]==expected,"source indexed pixels retain exact tile identity and offset"))return 1;
    }
    if(!check(TileValid(0,1,64)&&TileValid(512,1,64)&&TileValid(575,1,64)
              &&!TileValid(1,1,64)&&!TileValid(576,1,64)&&!TileValid(-1,512,512)&&!TileValid(1024,512,512),"missing source tiles are rejected, not clamped into unrelated art"))return 1;
    first[0]=16;
    if(!check(!CopyIndex(first.data(),first.size(),64,512,0,atlas.data(),atlas.size())
              &&!CopyIndex(second.data(),second.size(),255,128,512,atlas.data(),atlas.size())
              &&!CopyIndex(second.data(),second.size(),256,128,511,atlas.data(),atlas.size())
              &&!CopyIndex(second.data(),second.size(),256,128,512,atlas.data(),atlas.size()-1),"malformed indexed payloads and partitions are rejected"))return 1;

    std::puts("R5 metatile render math test passed.");
    return 0;
}
