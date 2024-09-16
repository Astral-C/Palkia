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
    while(stream.tell() < pos + commandsLen){
        uint8_t cmds[4] = { stream.readUInt8(), stream.readUInt8(), stream.readUInt8(), stream.readUInt8() };

        struct {
            Vertex vtx {};
            glm::mat4 mat {};
        } ctx;

        Primitive currentPrimitive { PrimitiveType::None };

        for(int c = 0; c < 4; c++){
            switch(cmds[c]){
                case 0x40:
                    currentPrimitive = { (PrimitiveType)(stream.readUInt32() & 0x03) };
                    break;
                
                case 0x41:
                    mPrimitives.push_back(currentPrimitive);
                    break;

                case 0x23: {
                    uint32_t a = stream.readUInt32();
                    uint32_t b = stream.readUInt32();
                    
                    glm::vec3 vtx;
                    vtx.x = ((a & 0xFFFF) << 16 >> 16);
                    vtx.y = (((a >> 16) & 0xFFFF) << 16 >> 16);
                    vtx.z = ((b & 0xFFFF) << 16 >> 16);

                    vtx /= 4096.0;
                    ctx.vtx.position = vtx;
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x24: {
                    uint32_t a = stream.readUInt32();

                    glm::vec3 vtx;
                    vtx.x = (a & 0x03FF) << 22 >> 22;
                    vtx.y = ((a >> 10) & 0x03FF) << 22 >> 22;
                    vtx.z = ((a >> 20) & 0x03FF) << 22 >> 22;

                    vtx /= 64.0;
                    ctx.vtx.position = vtx;
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x25: {
                    uint32_t a = stream.readUInt32();
                    
                    glm::vec3 vtx;
                    vtx.x = ((a & 0xFFFF) << 16 >> 16);
                    vtx.y = (((a >> 16) & 0xFFFF) << 16 >> 16);

                    vtx /= 4096.0;
                    ctx.vtx.position = glm::vec3(vtx.x, vtx.y, ctx.vtx.position.z);
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x26: {
                    uint32_t a = stream.readUInt32();
                    
                    glm::vec3 vtx;
                    vtx.x = ((a & 0xFFFF) << 16 >> 16);
                    vtx.z = (((a >> 16) & 0xFFFF) << 16 >> 16);

                    vtx /= 4096.0;
                    ctx.vtx.position = glm::vec3(vtx.x, ctx.vtx.position.y, vtx.z);
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x27: {
                    uint32_t a = stream.readUInt32();
                    
                    glm::vec3 vtx;
                    vtx.y = ((a & 0xFFFF) << 16 >> 16);
                    vtx.z = (((a >> 16) & 0xFFFF) << 16 >> 16);

                    vtx /= 4096.0;
                    ctx.vtx.position = glm::vec3(ctx.vtx.position.x, vtx.y, vtx.z);
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x28: {
                    uint32_t a = stream.readUInt32();

                    glm::vec3 vtx;
                    vtx.x = (a & 0x03FF) << 22 >> 22;
                    vtx.y = ((a >> 10) & 0x03FF) << 22 >> 22;
                    vtx.z = ((a >> 20) & 0x03FF) << 22 >> 22;

                    vtx /= 4096.0;
                    ctx.vtx.position += vtx;
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x21: {
                    uint32_t a = stream.readUInt32();

                    glm::vec3 vtx;
                    vtx.x = (a & 0x03FF) << 22 >> 22;
                    vtx.y = ((a >> 10) & 0x03FF) << 22 >> 22;
                    vtx.z = ((a >> 20) & 0x03FF) << 22 >> 22;

                    vtx /= 1024.0;
                    ctx.vtx.normal = vtx;
                    break;
                }

                case 0x20: {
                    uint32_t a = stream.readUInt32();

                    glm::vec3 vtx;
                    vtx.b = (cv5To8(a) & 0x1F) / 0xFF;
                    vtx.g = (cv5To8(a >> 5) & 0x1F) / 0xFF;
                    vtx.r = (cv5To8(a >> 10) & 0x1F) / 0xFF;

                    ctx.vtx.color = vtx;
                    break;
                }

                case 0x22: {
                    uint32_t a = stream.readUInt32();

                    glm::vec2 tc;

                    tc.x = ((a & 0xFFFF) << 16 >> 16) / 16.0;
                    tc.y = ((a >> 16) << 16 >> 16) / 16.0;

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

                default:
                    stream.readUInt32();
                    break;

            }
        }

    }

}

Model::Model(bStream::CStream& stream){
    size_t modelOffset = stream.tell();
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

        stream.seek(offset + meshesOffset);

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
            case 0x304C444D: {  // MDL0, why is this the wrong way???? '0LDM'
                std::cout << "Reading MDL0 at " << std::hex << stream.tell() << std::endl;
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
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<std::array<std::array<int, 3>, 3>> faces;

    std::stringstream posStream;
    std::stringstream normStream;
    std::stringstream texcStream;
    std::stringstream faceStream;

    for(int mod = 0; mod < mModels.size(); mod++){
        auto model = mModels.GetItems()[mod];

        for(int mes = 0; mes < model.GetMeshes().size(); mes++){
            auto mesh = model.GetMeshes().GetItems()[mes]; 
            for(auto prim : mesh.GetPrimitives()){
                auto vertices = prim.GetVertices();
                if(prim.GetType() == MDL0::Triangles){
                    for(int v = 0; v < prim.GetVertices().size() / 3; v++){
                        std::array<std::array<int, 3>, 3> face;
                        for(int i = 0; i < 3; i++){
                            face[i][0] = positions.size();
                            face[i][1] = normals.size();
                            face[i][2] = texcoords.size();
                            positions.push_back(vertices[(v * 3) + i].position);
                            normals.push_back(vertices[(v * 3) + i].normal);
                            texcoords.push_back(vertices[(v * 3) + i].texcoord);                    
                        }
                        faces.push_back(face);
                    }
                } else if(prim.GetType() == MDL0::Tristrips){
                    for(int v = 2; v < prim.GetVertices().size(); v++){
                        std::array<std::array<int, 3>, 3> face;
                        
                        face[0][0] = positions.size();
                        face[0][1] = normals.size();
                        face[0][2] = texcoords.size();
                        positions.push_back(vertices[v-2].position);
                        normals.push_back(vertices[v-2].normal);
                        texcoords.push_back(vertices[v-2].texcoord);                    
                        
                        face[1][0] = positions.size();
                        face[1][1] = normals.size();
                        face[1][2] = texcoords.size();
                        positions.push_back(vertices[(v % 2 != 0 ? v : v-1)].position);
                        normals.push_back(vertices[(v % 2 != 0 ? v : v-1)].normal);
                        texcoords.push_back(vertices[(v % 2 != 0 ? v : v-1)].texcoord);

                        face[2][0] = positions.size();
                        face[2][1] = normals.size();
                        face[2][2] = texcoords.size();
                        positions.push_back(vertices[(v % 2 != 0 ? v-1 : v)].position);
                        normals.push_back(vertices[(v % 2 != 0 ? v-1 : v)].normal);
                        texcoords.push_back(vertices[(v % 2 != 0 ? v-1 : v)].texcoord);

                        faces.push_back(face);
                    }
                }
            }

            for(auto p : positions){
                std::cout << p.x << std::endl;
                posStream << "v " << p.x << " " << p.y << " " << p.z << std::endl;
            }

            for(auto n : normals){
                normStream << "vn " << n.x << " " << n.y << " " << n.z << std::endl;
            }

            for(auto t : texcoords){
                texcStream << "vt " << t.x << " " << t.y << std::endl;
            }

            faceStream << "o " << std::string(mModels.GetNames()[mod].data()) << "." << (model.GetMeshes().GetNames()[mes].data()) << std::endl;
            for(auto f : faces){
                faceStream << "f " << f[0][0] << "/" << f[0][2] << "/" << f[0][1] << " "
                                << f[1][0] << "/" << f[1][2] << "/" << f[1][1] << " "
                                << f[2][0] << "/" << f[2][2] << "/" << f[2][1] << " " << std::endl;
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