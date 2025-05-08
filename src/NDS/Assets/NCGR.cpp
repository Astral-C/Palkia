#include <cstdint>
#include <vector>
#include <glad/glad.h>
#include <NDS/Assets/NCGR.hpp>
#include <Util.hpp>
#include <string>
#include <fstream>
#include <algorithm>

namespace Palkia {
namespace Formats {

std::vector<uint8_t> NCGR::Convert(uint32_t w, uint32_t h,  NCLR& pal){
    std::vector<uint8_t> image;
    image.resize(w * h * 4);

    uint32_t t = 0;
    for (int ty = 0; ty < h; ty+=8){
        for (int tx = 0; tx < w; tx+=8){
            for (int y = 0; y < 8; y++){
                for (int x = 0; x < 8; x++){
                    glm::vec3 color = pal[mTiles[t][(y * 8) + x]];
                    int dst = ((ty + y) * w + (tx + x)) * 4;
                    if(tx + x < w && ty + y < h){
                        image[dst]   = color.r;
                        image[dst+1] = color.g;
                        image[dst+2] = color.b;
                        image[dst+3] = mTiles[t][(y * 8) + x] == 0 ? 0x00 : 0xFF;
                    }
                }
            }
            t++;
        }
    }
    return image;
}

void NCGR::Load(bStream::CStream& stream){
    stream.readUInt32(); //magic
    stream.readUInt32(); // constant
    stream.readUInt32(); // section size

    stream.readUInt16(); // 0x10, size of this header
    stream.readUInt16(); // section count, 2 here

    {
        stream.readUInt32(); //section magic
        stream.readUInt32(); //section size

        mWidth = stream.readUInt16(); // width in tiles
        mHeight = stream.readUInt16(); // height in tiles

        mBitDepth = stream.readUInt32();

        stream.skip(8);
        uint32_t tileDataSize = stream.readUInt32();
        uint32_t tileDataOffset = stream.readUInt32();

        uint32_t readOffset = stream.tell();

        mTiles.resize(mBitDepth == 3 ? tileDataSize / 32 : tileDataSize / 64);
        stream.seek(tileDataOffset+24);

        for (int i = 0; i < mTiles.size(); i++) {
            if(mBitDepth == 3){
                for (int j = 0; j < 32; j++){
                    uint8_t byte = stream.readUInt8();
                    mTiles[i][j * 2] = byte & 0xF;
                    mTiles[i][j * 2 + 1] = byte >> 4;
                }
            } else if(mBitDepth == 4) {
                for(int t = 0; t < 64; t++){
                    mTiles[i][t] = stream.readUInt8();
                }
            }
        }

        stream.seek(readOffset);
    }

    { // most of this is unused
        stream.readUInt32(); //section magic
        stream.readUInt32(); // size, always 12
        stream.readUInt32();
        stream.readUInt16();
        stream.readUInt16();
    }

}

}
}
