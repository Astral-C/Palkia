#include <vector>
#include <Models/NSBMD.hpp>
#include <Util.hpp>
#include <string>
#include <sstream>
#include <fstream>

namespace Palkia {

namespace MDL0 {

uint8_t cv5To8(uint8_t v){
    return (v << (8 - 5)) | (v >> (10 - 8));
}

Mesh::Mesh(bStream::CStream& stream){
    size_t meshStart = stream.tell();
    std::cout << "Reading Mesh at " << std::hex << meshStart << std::endl;
    stream.readUInt16(); // dummy
    stream.readUInt16(); // size
    stream.readUInt32(); // unk

    uint32_t commandsOffset = stream.readUInt32() + meshStart;
    uint32_t commandsLen = stream.readUInt32();

    stream.seek(commandsOffset);

    size_t pos = stream.tell();
    std::cout << "Reading Geometry Commands at " << std::hex << pos << std::endl;
    Primitive currentPrimitive {};
    currentPrimitive.SetType(PrimitiveType::None);
    while(stream.tell() < pos + commandsLen){
        uint8_t cmds[4] = { stream.readUInt8(), stream.readUInt8(), stream.readUInt8(), stream.readUInt8() };

        struct {
            Vertex vtx {};
            glm::mat4 mat {};
        } ctx;


        for(int c = 0; c < 4; c++){
            switch(cmds[c]){
                case 0x40: {
                        uint32_t mode = stream.readUInt32();
                        std::cout << "Starting primitive with type " << std::hex<< mode << std::endl;
                        currentPrimitive = {};
                        currentPrimitive.SetType(mode);
                    }
                    break;
                
                case 0x41:
                    std::cout << "Ending primitive with type " << std::hex << currentPrimitive.GetType() << std::endl;
                    mPrimitives.push_back(currentPrimitive);
                    break;

                case 0x23: {
                    uint32_t a = stream.readUInt32();
                    uint32_t b = stream.readUInt32();
                    
                    glm::vec3 vtx;
                    vtx.x = (float)((a & 0xFFFF) << 16 >> 16);
                    vtx.y = (float)(((a >> 16) & 0xFFFF) << 16 >> 16);
                    vtx.z = (float)((b & 0xFFFF) << 16 >> 16);

                    vtx /= 4096.0f;
                    ctx.vtx.position = vtx;
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x24: {
                    uint32_t a = stream.readUInt32();

                    glm::vec3 vtx;
                    vtx.x = (float)((a & 0x03FF) << 22 >> 22);
                    vtx.y = (float)(((a >> 10) & 0x03FF) << 22 >> 22);
                    vtx.z = (float)(((a >> 20) & 0x03FF) << 22 >> 22);

                    vtx /= 64.0f;
                    ctx.vtx.position = vtx;
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x25: {
                    uint32_t a = stream.readUInt32();
                    
                    glm::vec3 vtx;
                    vtx.x = (float)((a & 0xFFFF) << 16 >> 16);
                    vtx.y = (float)(((a >> 16) & 0xFFFF) << 16 >> 16);

                    vtx /= 4096.0f;
                    ctx.vtx.position = glm::vec3(vtx.x, vtx.y, ctx.vtx.position.z);
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x26: {
                    uint32_t a = stream.readUInt32();
                    
                    glm::vec3 vtx;
                    vtx.x = (float)((a & 0xFFFF) << 16 >> 16);
                    vtx.z = (float)(((a >> 16) & 0xFFFF) << 16 >> 16);

                    vtx /= 4096.0f;
                    ctx.vtx.position = glm::vec3(vtx.x, ctx.vtx.position.y, vtx.z);
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x27: {
                    uint32_t a = stream.readUInt32();
                    
                    glm::vec3 vtx;
                    vtx.y = (float)(((a & 0xFFFF) << 16 >> 16));
                    vtx.z = (float)((((a >> 16) & 0xFFFF) << 16 >> 16));

                    vtx /= 4096.0f;
                    ctx.vtx.position = glm::vec3(ctx.vtx.position.x, vtx.y, vtx.z);
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x28: {
                    uint32_t a = stream.readUInt32();

                    glm::vec3 vtx;
                    vtx.x = (float)((a & 0x03FF) << 22 >> 22);
                    vtx.y = (float)(((a >> 10) & 0x03FF) << 22 >> 22);
                    vtx.z = (float)(((a >> 20) & 0x03FF) << 22 >> 22);

                    vtx /= 4096.0;
                    ctx.vtx.position += vtx;
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x21: {
                    uint32_t a = stream.readUInt32();

                    glm::vec3 vtx;
                    vtx.x = (float)((a & 0x03FF) << 22 >> 22);
                    vtx.y = (float)(((a >> 10) & 0x03FF) << 22 >> 22);
                    vtx.z = (float)(((a >> 20) & 0x03FF) << 22 >> 22);

                    vtx /= 1024.0f;
                    ctx.vtx.normal = vtx;
                    break;
                }

                case 0x20: {
                    uint32_t a = stream.readUInt32();

                    glm::vec3 vtx;
                    vtx.b = (float)(cv5To8(a) & 0x1F) / 0xFF;
                    vtx.g = (float)(cv5To8(a >> 5) & 0x1F) / 0xFF;
                    vtx.r = (float)(cv5To8(a >> 10) & 0x1F) / 0xFF;

                    ctx.vtx.color = vtx;
                    break;
                }

                case 0x22: {
                    uint32_t a = stream.readUInt32();

                    glm::vec2 tc;

                    tc.x = (float)((a & 0xFFFF) << 16 >> 16) / 16.0f;
                    tc.y = (float)((a >> 16) << 16 >> 16) / 16.0f;

                    ctx.vtx.texcoord = tc;
                    break;
                }

                case 0x14: {
                    stream.readUInt32();
                    break;
                }

                case 0x30: {
                    stream.readUInt32();
                    break;
                }

                case 0x00:
                    break;

                default:
                    std::cout << "Uknown GPU Command 0x" << std::hex << cmds[c] << std::endl;
                    break;

            }
        }

    }

}

Model::Model(bStream::CStream& stream){
    size_t modelOffset = stream.tell();
    std::cout << "Reading model at " << std::hex << stream.tell() << std::endl;
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

    stream.seek(meshesOffset + modelOffset);
    std::cout << "Reading mesh list at " << std::hex << stream.tell() << std::endl;
    mMeshes.Load(stream, [&](bStream::CStream& stream){
        uint32_t offset = stream.readUInt32();
        size_t listPos = stream.tell();

        stream.seek(offset + meshesOffset + modelOffset);

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

        std::cout << "Reading Segment at " << std::hex << stream.tell() << std::endl;
        std::cout << "Stamp is " << stream.peekString(stream.tell(), 4) << std::endl;
        uint32_t stamp = stream.readUInt32();
        
        switch (stamp){
            case 0x304C444D: {  // MDL0, why is this the wrong way???? '0LDM'
                stream.readUInt32(); // section size
                std::cout << "Reading model list at " << std::hex << stream.tell() << std::endl;
                mModels.Load(stream, [&](bStream::CStream& stream){
                    uint32_t offset = stream.readUInt32();
                    size_t listPos = stream.tell();

                    stream.seek(sectionOffset + offset);

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

void NSBMD::Dump(){

    std::stringstream posStream;
    std::stringstream normStream;
    std::stringstream texcStream;
    std::stringstream faceStream;

    int pidx = 0;
    int nidx = 0;
    int tcidx = 0;

    for(int mod = 0; mod < mModels.size(); mod++){
        auto model = mModels.GetItems()[mod];

        for(int mes = 0; mes < model.GetMeshes().size(); mes++){
            auto mesh = model.GetMeshes().GetItems()[mes]; 
            faceStream << "o " << std::string(mModels.GetNames()[mod].data()) << "." << (model.GetMeshes().GetNames()[mes].data()) << std::endl;
            for(auto prim : mesh.GetPrimitives()){
                auto vertices = prim.GetVertices();

                if(prim.GetType() == MDL0::Triangles){
                    for(int v = 0; v < prim.GetVertices().size() / 3; v++){
                        auto vtx1 = vertices[(v * 3) + 0];
                        auto vtx2 = vertices[(v * 3) + 1];
                        auto vtx3 = vertices[(v * 3) + 2]; 

                        posStream << "v " << vtx1.position.x << " " << vtx1.position.y << " " << vtx1.position.z << std::endl;
                        posStream << "v " << vtx2.position.x << " " << vtx2.position.y << " " << vtx2.position.z << std::endl;
                        posStream << "v " << vtx3.position.x << " " << vtx3.position.y << " " << vtx3.position.z << std::endl;

                        normStream << "vn " << vtx1.normal.x << " " << vtx1.normal.y << " " << vtx1.normal.z << std::endl;
                        normStream << "vn " << vtx2.normal.x << " " << vtx2.normal.y << " " << vtx2.normal.z << std::endl;
                        normStream << "vn " << vtx3.normal.x << " " << vtx3.normal.y << " " << vtx3.normal.z << std::endl;

                        texcStream << "vt " << vtx1.texcoord.x << " " << vtx1.texcoord.y << std::endl;
                        texcStream << "vt " << vtx2.texcoord.x << " " << vtx2.texcoord.y << std::endl;
                        texcStream << "vt " << vtx3.texcoord.x << " " << vtx3.texcoord.y << std::endl;

                        faceStream << "f " << pidx   << "/" << nidx  << "/" << tcidx   << " " 
                                           << pidx+1 << "/" << nidx+1<< "/" << tcidx+1 << " " 
                                           << pidx+2 << "/" << nidx+2<< "/" << tcidx+2 << " " << std::endl;
                        
                        pidx += 3;
                        nidx += 3;
                        tcidx += 3;
                    }
                } else if(prim.GetType() == MDL0::Quads){
                    for(int v = 0; v < prim.GetVertices().size() / 4; v++){
                        auto vtx1 = vertices[(v * 4) + 0];
                        auto vtx2 = vertices[(v * 4) + 1];
                        auto vtx3 = vertices[(v * 4) + 2];
                        auto vtx4 = vertices[(v * 4) + 2];

                        posStream << "v " << vtx1.position.x << " " << vtx1.position.y << " " << vtx1.position.z << std::endl;
                        posStream << "v " << vtx2.position.x << " " << vtx2.position.y << " " << vtx2.position.z << std::endl;
                        posStream << "v " << vtx3.position.x << " " << vtx3.position.y << " " << vtx3.position.z << std::endl;

                        posStream << "v " << vtx2.position.x << " " << vtx2.position.y << " " << vtx2.position.z << std::endl;
                        posStream << "v " << vtx3.position.x << " " << vtx3.position.y << " " << vtx3.position.z << std::endl;
                        posStream << "v " << vtx4.position.x << " " << vtx4.position.y << " " << vtx4.position.z << std::endl;

                        normStream << "vn " << vtx1.normal.x << " " << vtx1.normal.y << " " << vtx1.normal.z << std::endl;
                        normStream << "vn " << vtx2.normal.x << " " << vtx2.normal.y << " " << vtx2.normal.z << std::endl;
                        normStream << "vn " << vtx3.normal.x << " " << vtx3.normal.y << " " << vtx3.normal.z << std::endl;

                        normStream << "vn " << vtx2.normal.x << " " << vtx2.normal.y << " " << vtx2.normal.z << std::endl;
                        normStream << "vn " << vtx3.normal.x << " " << vtx3.normal.y << " " << vtx3.normal.z << std::endl;
                        normStream << "vn " << vtx4.normal.x << " " << vtx4.normal.y << " " << vtx4.normal.z << std::endl;

                        texcStream << "vt " << vtx1.texcoord.x << " " << vtx1.texcoord.y << std::endl;
                        texcStream << "vt " << vtx2.texcoord.x << " " << vtx2.texcoord.y << std::endl;
                        texcStream << "vt " << vtx3.texcoord.x << " " << vtx3.texcoord.y << std::endl;

                        texcStream << "vt " << vtx2.texcoord.x << " " << vtx2.texcoord.y << std::endl;
                        texcStream << "vt " << vtx3.texcoord.x << " " << vtx3.texcoord.y << std::endl;
                        texcStream << "vt " << vtx4.texcoord.x << " " << vtx4.texcoord.y << std::endl;
                        
                        faceStream << "f " << pidx   << "/" << nidx  << "/" << tcidx   << " " 
                                           << pidx+1 << "/" << nidx+1<< "/" << tcidx+1 << " " 
                                           << pidx+2 << "/" << nidx+2<< "/" << tcidx+2 << " " << std::endl;
                        
                        faceStream << "f " << pidx+1 << "/" << nidx+1<< "/" << tcidx+1 << " " 
                                           << pidx+2 << "/" << nidx+2<< "/" << tcidx+2 << " " 
                                           << pidx+3 << "/" << nidx+3<< "/" << tcidx+3 << " " << std::endl;

                        pidx += 4;
                        nidx += 4;
                        tcidx += 4;
                    }
                } else if(prim.GetType() == MDL0::Tristrips){
                    for(int v = 2; v < prim.GetVertices().size(); v++){
                        auto vtx1 = vertices[v-1];
                        auto vtx2 = vertices[(v % 2 != 0 ? v : v-1)];
                        auto vtx3 = vertices[(v % 2 != 0 ? v-1 : v)]; 

                        posStream << "v " << vtx1.position.x << " " << vtx1.position.y << " " << vtx1.position.z << std::endl;
                        posStream << "v " << vtx2.position.x << " " << vtx2.position.y << " " << vtx2.position.z << std::endl;
                        posStream << "v " << vtx3.position.x << " " << vtx3.position.y << " " << vtx3.position.z << std::endl;

                        normStream << "vn " << vtx1.normal.x << " " << vtx1.normal.y << " " << vtx1.normal.z << std::endl;
                        normStream << "vn " << vtx2.normal.x << " " << vtx2.normal.y << " " << vtx2.normal.z << std::endl;
                        normStream << "vn " << vtx3.normal.x << " " << vtx3.normal.y << " " << vtx3.normal.z << std::endl;

                        texcStream << "vt " << vtx1.texcoord.x << " " << vtx1.texcoord.y << std::endl;
                        texcStream << "vt " << vtx2.texcoord.x << " " << vtx2.texcoord.y << std::endl;
                        texcStream << "vt " << vtx3.texcoord.x << " " << vtx3.texcoord.y << std::endl;

                        faceStream << "f " << pidx   << "/" << nidx  << "/" << tcidx   << " " 
                                           << pidx+1 << "/" << nidx+1<< "/" << tcidx+1 << " " 
                                           << pidx+2 << "/" << nidx+2<< "/" << tcidx+2 << " " << std::endl;
                        
                        pidx += 3;
                        nidx += 3;
                        tcidx += 3;
                    }
                }
            }
        }
    }

    std::ofstream outObj("dump.obj");
    outObj << "# written by palkia" << std::endl;

    outObj << posStream.str() << std::endl;
    outObj << normStream.str() << std::endl;
    outObj << texcStream.str() << std::endl << std::endl;
    outObj << faceStream.str() << std::endl;
}

NSBMD::NSBMD(){

}

NSBMD::~NSBMD(){

}

}