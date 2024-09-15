#include <vector>
#include <Models/NSBMD.hpp>
#include <Util.hpp>

namespace Palkia {

namespace MDL0 {

Mesh::Mesh(bStream::CStream& stream){
    stream.readUInt16(); // dummy
    stream.readUInt16(); // size
    stream.readUInt32(); // unk

    uint32_t commandsOffset = stream.readUInt32();
    uint32_t commandsLen = stream.readUInt32();

    stream.seek(commandsOffset);

    size_t pos = stream.tell();
    while(stream.tell() - pos < commandsLen){
        uint8_t cmds[4] = { stream.readUInt8(), stream.readUInt8(), stream.readUInt8(), stream.readUInt8() };

        struct {
            glm::vec3 position {};
            glm::vec4 color {};
            glm::vec3 normal {};
            glm::mat4 mat {};
        } ctx;

        for(int c = 0; c < 4; c++){
            switch(cmds[c]){
                case 0x41:
                    break;
            }
        }

    }

}

Model::Model(bStream::CStream& stream){
    stream.readUInt32(); // size of MDL0?
    uint32_t renderCMDOffset = stream.readUInt32(); 
    uint32_t materialsOffset = stream.readUInt32();
    uint32_t meshesOffset = stream.readUInt32();
    uint32_t inverseBindMatsOffset = stream.readUInt32();

    stream.skip(3);
    uint8_t numBoneMatrices = stream.readUInt8();
    uint8_t numMaterials = stream.readUInt8();
    uint8_t numMeshes = stream.readUInt8();
    stream.skip(2);

    //apparently unused
    stream.readUInt32(); // up scale
    stream.readUInt32(); // down scale

    stream.readUInt16(); // num verts
    stream.readUInt16(); // num polys
    stream.readUInt16(); // num tris
    stream.readUInt16(); // num quads

    uint16_t minX = stream.readUInt16();
    uint16_t minY = stream.readUInt16();
    uint16_t minZ = stream.readUInt16();

    uint16_t maxX = minX + stream.readUInt16();
    uint16_t maxY = minY + stream.readUInt16();
    uint16_t maxZ = minZ + stream.readUInt16();

    stream.skip(8);

    stream.seek(meshesOffset);
    mMeshes.Load(stream, [](bStream::CStream& stream){
        uint32_t offset = stream.readUInt32();
        size_t listPos = stream.tell();

        stream.seek(offset);

        Mesh mesh(stream);

        stream.seek(listPos);
        return  mesh;
    });

}

}


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

                mModels.Load(stream, [](bStream::CStream& stream){
                    uint32_t offset = stream.readUInt32();
                    size_t listPos = stream.tell();

                    stream.seek(offset);

                    MDL0::Model model(stream);

                    stream.seek(listPos);
                    return model;
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