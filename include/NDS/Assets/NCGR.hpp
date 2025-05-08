#pragma once
#include <Util.hpp>
#include <bstream/bstream.h>
#include <NDS/Assets/NCLR.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <map>
#include <array>

namespace Palkia {

namespace Formats {

class NCGR {
    uint32_t mBitDepth { 3 };
    std::vector<std::array<uint8_t, 64>> mTiles;
public:
    uint16_t mWidth, mHeight;
    std::vector<uint8_t> Convert(uint32_t w, uint32_t h, NCLR& pal);
    void Load(bStream::CStream& stream);

    NCGR(){}
    ~NCGR(){}
};

}

}
