#include <vector>
#include <glad/glad.h>
#include <Models/NSBMD.hpp>
#include <Util.hpp>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

namespace Palkia {

namespace MDL0 {

const char* default_vtx_shader_source = "#version 460\n\
    #extension GL_ARB_separate_shader_objects : enable\n\
    layout(location = 0) in vec3 inPosition;\n\
    layout(location = 1) in vec3 inNormal;\n\
    layout(location = 2) in vec3 inColor;\n\
    layout(location = 3) in vec2 inTexCoord;\n\
    \
    uniform mat4 transform;\n\
    \
    layout(location = 0) out vec2 fragTexCoord;\n\
    layout(location = 1) out vec3 fragVtxColor;\n\
    \
    void main()\n\
    {\
        gl_Position = transform * vec4(inPosition, 1.0);\n\
        fragVtxColor = inColor;\n\
        fragTexCoord = inTexCoord;\n\
    }\
";

//uniform int pickID;\n
const char* default_frg_shader_source = "#version 460\n\
    #extension GL_ARB_separate_shader_objects : enable\n\
    \
    uniform sampler2D texSampler;\n\
    layout(location = 0) in vec2 fragTexCoord;\n\
    layout(location = 1) in vec3 fragVtxColor;\n\
    \
    layout(location = 0) out vec4 outColor;\n\
    layout(location = 1) out int outPick;\n\
    \
    void main()\n\
    {\n\
        outColor = vec4(2.0, 2.0, 2.0, 1.0) * vec4(fragVtxColor.xyz, 1.0);\n\
    }\
";

static uint32_t mProgram { 0xFFFFFFFF };

void DestroyShaders(){
    glDeleteProgram(mProgram);
}

void InitShaders(){
    char glErrorLogBuffer[4096];
    uint32_t vs = glCreateShader(GL_VERTEX_SHADER);
    uint32_t fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vs, 1, &default_vtx_shader_source, NULL);
    glShaderSource(fs, 1, &default_frg_shader_source, NULL);
    glCompileShader(vs);
    uint32_t status;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE){
        uint32_t infoLogLength;
        glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &infoLogLength);
        glGetShaderInfoLog(vs, infoLogLength, NULL, glErrorLogBuffer);
        printf("[NSBMD Loader]: Compile failure in mdl vertex shader:\n%s\n", glErrorLogBuffer);
    }
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE){
        uint32_t infoLogLength;
        glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &infoLogLength);
        glGetShaderInfoLog(fs, infoLogLength, NULL, glErrorLogBuffer);
        printf("[NSBMD Loader]: Compile failure in mdl fragment shader:\n%s\n", glErrorLogBuffer);
    }
    mProgram = glCreateProgram();
    glAttachShader(mProgram, vs);
    glAttachShader(mProgram, fs);
    glLinkProgram(mProgram);
    glGetProgramiv(mProgram, GL_LINK_STATUS, &status); 
    if(GL_FALSE == status) {
        uint32_t logLen; 
        glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &logLen); 
        glGetProgramInfoLog(mProgram, logLen, NULL, glErrorLogBuffer); 
        printf("[NSBMD Loader]: Shader Program Linking Error:\n%s\n", glErrorLogBuffer);
    } 
    glDetachShader(mProgram, vs);
    glDetachShader(mProgram, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

uint8_t cv5To8(uint8_t v){
    return (v << (8 - 5)) | (v >> (10 - 8));
}

void Primitive::GenerateBuffers(){
    glGenVertexArrays(1, &mVao);
    glBindVertexArray(mVao);

    glGenBuffers(1, &mVbo);
    glBindBuffer(GL_ARRAY_BUFFER, mVbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));

    glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(Vertex), mVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Primitive::Render(){
    glBindVertexArray(mVao);

    if(mType == PrimitiveType::Triangles){
        glDrawArrays(GL_TRIANGLES, 0, mVertices.size());
    } else if(mType == PrimitiveType::Quads){
        glDrawArrays(GL_QUADS, 0, mVertices.size());
    } else if(mType == PrimitiveType::Tristrips){
        glDrawArrays(GL_TRIANGLE_STRIP, 0, mVertices.size());
    } else if(mType == PrimitiveType::Quadstrips){
        glDrawArrays(GL_QUAD_STRIP, 0, mVertices.size());
    }
}

void Mesh::Render(){
    for(auto primitive : mPrimitives){
        primitive.Render();
    }
}

void Model::Render(){
    for(auto mesh : mMeshes.GetItems()){
        mesh.Render();
    }
}

void NSBMD::Render(glm::mat4 v){
    if(MDL0::mProgram == 0xFFFFFFFF){
        MDL0::InitShaders();
    }

    glUseProgram(MDL0::mProgram);
    glUniformMatrix4fv(glGetUniformLocation(MDL0::mProgram, "transform"), 1, 0, &v[0][0]);

    for(auto model : mModels.GetItems()){
        model.Render();
    }
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
    
    struct {
        Vertex vtx {};
        glm::mat4 mat {};
    } ctx;

    while(stream.tell() < pos + commandsLen){
        uint8_t cmds[4] = { stream.readUInt8(), stream.readUInt8(), stream.readUInt8(), stream.readUInt8() };

        std::cout << std::endl << std::endl;
        for(int c = 0; c < 4; c++){
            std::cout << "Execing Command " << std::hex << (uint32_t)cmds[c] << std::endl;
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

                    currentPrimitive.GenerateBuffers();
                    mPrimitives.push_back(currentPrimitive);
                    break;

                case 0x23: {
                    uint32_t a = stream.readUInt32();
                    uint32_t b = stream.readUInt32();
                    
                    ctx.vtx.position.x = (int16_t)(((a & 0xFFFF) << 16) >> 16) / 4096.0f;
                    ctx.vtx.position.y = (int16_t)((((a >> 16) & 0xFFFF) << 16) >> 16) / 4096.0f;
                    ctx.vtx.position.z = (int16_t)(((b & 0xFFFF) << 16) >> 16) / 4096.0f;

                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x24: {
                    uint32_t a = stream.readUInt32();

                    ctx.vtx.position.x = (int16_t)(((a & 0x03FF) << 6) >> 6) / 64.0f;
                    ctx.vtx.position.y = (int16_t)((((a >> 10) & 0x03FF) << 6) >> 6) / 64.0f;
                    ctx.vtx.position.z = (int16_t)((((a >> 20) & 0x03FF) << 6) >> 6) / 64.0f;

                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x25: {
                    uint32_t a = stream.readUInt32();
                    
                    ctx.vtx.position.x = (int16_t)((((a >>  0) & 0xFFFF) << 16) >> 16) / 4096.0f;
                    ctx.vtx.position.y = (int16_t)((((a >> 16) & 0xFFFF) << 16) >> 16) / 4096.0f;

                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x26: {
                    uint32_t a = stream.readUInt32();

                    ctx.vtx.position.x = (int16_t)((((a >>  0) & 0xFFFF) << 16) >> 16) / 4096.0f;
                    ctx.vtx.position.z = (int16_t)((((a >> 16) & 0xFFFF) << 16) >> 16) / 4096.0f;
                    
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x27: {
                    uint32_t a = stream.readUInt32();

                    ctx.vtx.position.y = (int16_t)((((a >>  0) & 0xFFFF) << 16) >> 16) / 4096.0f;
                    ctx.vtx.position.z = (int16_t)((((a >> 16) & 0xFFFF) << 16) >> 16) / 4096.0f;
                    
                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x28: {
                    uint32_t a = stream.readUInt32();

                    glm::vec3 vtx;
                    ctx.vtx.position.x += ((int16_t)((((a >>  0) & 0x03FF) << 22) >> 22)) / 4096.0f;
                    ctx.vtx.position.y += ((int16_t)((((a >> 10) & 0x03FF) << 22) >> 22)) / 4096.0f;
                    ctx.vtx.position.z += ((int16_t)((((a >> 20) & 0x03FF) << 22) >> 22)) / 4096.0f;

                    currentPrimitive.Push(ctx.vtx);
                    break;
                }

                case 0x21: {
                    uint32_t a = stream.readUInt32();

                    glm::vec3 vtx;
                    vtx.x = ((int16_t)(((a & 0x03FF) << 22) >> 22)) / 1024.0f;
                    vtx.y = ((int16_t)((((a >> 10) & 0x03FF) << 22) >> 22)) / 1024.0f;
                    vtx.z = ((int16_t)((((a >> 20) & 0x03FF) << 22) >> 22)) / 1024.0f;

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

                    tc.x = (float)((int16_t)((a & 0xFFFF) << 16) >> 16) / 16.0f;
                    tc.y = (float)((int16_t)((a >> 16) << 16) >> 16) / 16.0f;

                    ctx.vtx.texcoord = tc;
                    break;
                }

                case 0x14:{
                    std::cout << "restore mtx" << std::endl;
                    stream.readUInt32();
                    break;
                } 
                case 0x1b: {
                    std::cout << "scale" << std::endl;
                    stream.readUInt32();
                    stream.readUInt32();
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

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    std::stringstream posStream;
    std::stringstream normStream;
    std::stringstream texcStream;
    std::stringstream faceStream;

    for(int mod = 0; mod < mModels.size(); mod++){
        auto model = mModels.GetItems()[mod];

        for(int mes = 0; mes < model.GetMeshes().size(); mes++){
            auto mesh = model.GetMeshes().GetItems()[mes]; 
            faceStream << "o " << std::string(mModels.GetNames()[mod].data()) << "." << (model.GetMeshes().GetNames()[mes].data()) << std::endl;
            for(auto prim : mesh.GetPrimitives()){
                auto vertices = prim.GetVertices();

                if(prim.GetType() == MDL0::Triangles){
                    for(int v = 0; v < prim.GetVertices().size(); v+=3){
                        std::array<MDL0::faceVtx, 3> tri;
                        for(int i = 0; i < 3; i++){
                            MDL0::faceVtx triVtx;
                            auto vtx = vertices[v + i];

                            auto pos = std::find(positions.begin(), positions.end(), vtx.position);
                            
                            if(pos != positions.end()){
                                triVtx.posIdx = pos - positions.begin();
                            } else {
                                triVtx.posIdx = positions.size();
                                positions.push_back(vtx.position);
                            }

                            auto norm = std::find(normals.begin(), normals.end(), vtx.normal);
                            
                            if(norm != normals.end()){
                                triVtx.normalIdx = norm - normals.begin();
                            } else {
                                triVtx.normalIdx = normals.size();
                                normals.push_back(vtx.normal);
                            }

                            auto texco = std::find(texcoords.begin(), texcoords.end(), vtx.texcoord);
                            
                            if(texco != texcoords.end()){
                                triVtx.texcoordIdx = texco - texcoords.begin();
                            } else {
                                triVtx.texcoordIdx = texcoords.size();
                                texcoords.push_back(vtx.texcoord);
                            }
                            tri[i] = triVtx;
                        }
                        faceStream << "f "
                                   << tri[0].posIdx << "/" << tri[0].texcoordIdx << "/" << tri[0].normalIdx << " "
                                   << tri[1].posIdx << "/" << tri[1].texcoordIdx << "/" << tri[1].normalIdx << " "
                                   << tri[2].posIdx << "/" << tri[2].texcoordIdx << "/" << tri[2].normalIdx << std::endl;
                    }
                } else if(prim.GetType() == MDL0::Quads){
                    for(int v = 0; v < prim.GetVertices().size(); v+=4){
                        std::array<MDL0::faceVtx, 3> tri;
                        for(int i = 0; i < 3; i++){
                            MDL0::faceVtx triVtx;
                            auto vtx = vertices[v + i];

                            triVtx.posIdx = positions.size();
                            positions.push_back(vtx.position);
                            
                            triVtx.normalIdx = normals.size();
                            normals.push_back(vtx.normal);

                            triVtx.texcoordIdx = texcoords.size();
                            texcoords.push_back(vtx.texcoord);
                            tri[i] = triVtx;
                        }

                        faceStream << "f "
                                   << tri[0].posIdx << "/" << tri[0].texcoordIdx << "/" << tri[0].normalIdx << " "
                                   << tri[1].posIdx << "/" << tri[1].texcoordIdx << "/" << tri[1].normalIdx << " "
                                   << tri[2].posIdx << "/" << tri[2].texcoordIdx << "/" << tri[2].normalIdx << std::endl;

                        for(int i = 2; i < 4; i++){
                            MDL0::faceVtx triVtx;
                            auto vtx = vertices[v + i];

                            triVtx.posIdx = positions.size();
                            positions.push_back(vtx.position);
                            
                            triVtx.normalIdx = normals.size();
                            normals.push_back(vtx.normal);

                            triVtx.texcoordIdx = texcoords.size();
                            texcoords.push_back(vtx.texcoord);
                            tri[i-1];
                        }
                        faceStream << "f "
                                   << tri[0].posIdx << "/" << tri[0].texcoordIdx << "/" << tri[0].normalIdx << " "
                                   << tri[1].posIdx << "/" << tri[1].texcoordIdx << "/" << tri[1].normalIdx << " "
                                   << tri[2].posIdx << "/" << tri[2].texcoordIdx << "/" << tri[2].normalIdx << std::endl;
                    }
                } else if(prim.GetType() == MDL0::Tristrips || prim.GetType() == MDL0::Quadstrips){
                    for(int v = 2; v < prim.GetVertices().size(); v++){
                        std::vector<MDL0::Vertex> verts = { vertices[v-1], vertices[(v % 2 != 0 ? v : v-1)], vertices[(v % 2 != 0 ? v-1 : v)] }; 
                        std::array<MDL0::faceVtx, 3> tri;

                        for(int i = 0; i < 3; i++){
                            MDL0::faceVtx triVtx;
                            auto vtx = verts[i];

                            triVtx.posIdx = positions.size();
                            positions.push_back(vtx.position);

                            triVtx.normalIdx = normals.size();
                            normals.push_back(vtx.normal);

                            triVtx.texcoordIdx = texcoords.size();
                            texcoords.push_back(vtx.texcoord);
                            tri[i] = triVtx;
                        }

                        faceStream << "f "
                                   << tri[0].posIdx << "/" << tri[0].texcoordIdx << "/" << tri[0].normalIdx << " "
                                   << tri[1].posIdx << "/" << tri[1].texcoordIdx << "/" << tri[1].normalIdx << " "
                                   << tri[2].posIdx << "/" << tri[2].texcoordIdx << "/" << tri[2].normalIdx << std::endl;
                    }
                }
            }
        }
    }

    for(auto p : positions){
        posStream << "v " << p.x << " " << p.y << " " << p.z << std::endl;
    }

    for(auto n : normals){
        posStream << "vn " << n.x << " " << n.y << " " << n.z << std::endl;
    }

    for(auto t : texcoords){
        posStream << "vt " << t.x << " " << t.y << std::endl;
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