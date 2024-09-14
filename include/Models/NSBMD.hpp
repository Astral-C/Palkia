#pragma once
#include <bstream/bstream.h>

namespace Palkia {

class Mesh {

};

class NSBMD {
private:
        
public:
    void Load(bStream::CStream& stream);
    NSBMD();
    ~NSBMD();
};

}