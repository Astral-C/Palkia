#include <vector>
#include <Models/NSBMD.hpp>
#include <Util.hpp>

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

        std::cout << "Stamp is " << stream.peekString(stream.tell(), 4) << std::endl;
        uint32_t stamp = stream.readUInt32();
        
        switch (stamp){
            case '0LDM': {  // MDL0, why is this the wrong way????
                uint32_t sectionSize = stream.readUInt32();

                Nitro::List<Mesh> meshOffsets(stream, [](bStream::CStream& stream){
                    uint32_t offset = stream.readUInt32();
                    size_t listPos = stream.tell();

                    stream.seek(offset);
                    // read mesh

                    stream.seek(listPos);
                });


                break;
            }

            default:
                break;
        }
    
        stream.seek(returnOffset);

    }

}

NSBMD::NSBMD(){

}

NSBMD::~NSBMD(){

}

}