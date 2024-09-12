#include <vector>
#include <Models/NSBMD.hpp>

namespace Palkia {

void NSBMD::Load(bStream::CStream& stream){
    stream.readUInt32(); // stamp
    stream.readUInt16(); // byte order

    stream.readUInt16(); // ver
    stream.readUInt32(); // filesize

    stream.readUInt16(); // header size

    uint16_t sectionCount = stream.readUInt16();

    for (size_t i = 0; i < sectionCount; i++){
        uint32_t sectionOffset = stream.readUInt32();

        uint32_t returnOffset = stream.tell();

        stream.seek(sectionOffset);

        uint32_t stamp = stream.readUInt32();

        switch (stamp){
            case 'MDL0': {    
                uint32_t sectionSize = stream.readUInt32();

                //List<Model?

                break;
            }

            default:
                break;
        }
    
    
    }


}

NSBMD::NSBMD(){

}

NSBMD::~NSBMD(){

}

}