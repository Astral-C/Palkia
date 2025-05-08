#include <vector>
#include <glad/glad.h>
#include <NDS/Assets/NCLR.hpp>
#include <Util.hpp>
#include <string>
#include <fstream>
#include <algorithm>

namespace Palkia {
namespace Formats {

void NCLR::Load(bStream::CStream& stream){
    stream.readUInt32(); //magic
    stream.readUInt32(); // constant
    stream.readUInt32(); // section size

    stream.readUInt16(); // 0x10, size of this header
    stream.readUInt16(); // section count, 2 here

    std::vector<glm::vec3> colors;

    {
        stream.readUInt32(); //section magic
        stream.readUInt32(); // size
        uint32_t bitDepth = stream.readUInt32();
        stream.readUInt32(); // 0
        uint32_t paletteDataSize = stream.readUInt32();

        if(0x200 - paletteDataSize > 0){
            paletteDataSize = 0x200 - paletteDataSize;
        }

        if(bitDepth == 4){
            paletteDataSize = 0x200;
        }

        uint32_t colorCount = stream.readUInt32();

        for(int i = 0; i < colorCount; i++){
            uint16_t color = stream.readUInt16();
            uint8_t r = cv5To8(color & 0x1F);
            uint8_t g = cv5To8((color >> 5) & 0x1F);
            uint8_t b = cv5To8((color >> 10) & 0x1F);
            mColors.push_back({r, g, b});
        }
    }

    {
        stream.readUInt32(); //section magic
        stream.readUInt32(); // size, always 12

        uint32_t paletteCount = stream.readUInt32();
        stream.skip(6);
        for(int i = 0; i < paletteCount; i++){
            //mColors.insert({stream.readUInt16(), colors[i]});
        }
    }

}

}
}
