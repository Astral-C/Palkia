#pragma once
#include <Util.hpp>
#include <bstream/bstream.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <map>
#include <array>

namespace Palkia {

namespace Formats {

class NCLR {
    std::vector<glm::vec3> mColors;
public:

    glm::vec3 operator[](uint16_t id){ return mColors[id]; }

    void Load(bStream::CStream& stream);

    NCLR(){}
    ~NCLR(){}
};

}

}
