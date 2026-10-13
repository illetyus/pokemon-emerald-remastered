#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>

// Pinned fieldmap.h VRAM/palette partitions; presentation never changes save state.
namespace RemasterIndexedMap {
inline constexpr int PrimaryTiles=512, TotalTiles=1024, PrimaryPalettes=6;
inline constexpr int TilePixels=8, AtlasWidth=128, AtlasHeight=512;
inline bool TileValid(int id,int primaryCount,int secondaryCount) {
    return id>=0 && id<TotalTiles && (id<PrimaryTiles ? id<primaryCount : id-PrimaryTiles<secondaryCount);
}
inline bool CopyIndex(const std::uint8_t* source,std::size_t bytes,int width,int height,
                      int firstTile,std::uint8_t* atlas,std::size_t atlasBytes) {
    if(!source || !atlas || width<=0 || height<=0 || width%TilePixels || height%TilePixels
       || bytes!=static_cast<std::size_t>(width)*static_cast<std::size_t>(height)
       || atlasBytes!=AtlasWidth*AtlasHeight || (firstTile!=0 && firstTile!=PrimaryTiles)) return false;
    const int columns=width/TilePixels;
    const int count=static_cast<int>(std::min<std::size_t>(PrimaryTiles,bytes/64));
    for(int tile=0;tile<count;++tile) for(int y=0;y<TilePixels;++y) for(int x=0;x<TilePixels;++x) {
        const auto value=source[static_cast<std::size_t>(tile/columns*TilePixels+y)*width+tile%columns*TilePixels+x];
        if(value>15)return false;
        const int target=firstTile+tile;
        atlas[(target/(AtlasWidth/TilePixels)*TilePixels+y)*AtlasWidth+target%(AtlasWidth/TilePixels)*TilePixels+x]=value;
    }
    return true;
}
}
