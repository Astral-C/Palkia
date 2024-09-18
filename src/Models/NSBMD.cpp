#include <vector>
#include <glad/glad.h>
#include <Models/NSBMD.hpp>
#include <Util.hpp>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <format>

namespace Palkia {

uint8_t cv5To8(uint8_t v){
    return (v << 3) | (v >> 2);
}

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
        vec4 texel = texture(texSampler, fragTexCoord);\n\
        outColor = texel * vec4(fragVtxColor.xyz, 1.0);\n\
        outPick = 0;\n\
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

namespace MDL0 {

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
    // render based on render commands
    for(auto [name, mesh] : mMeshes.GetItems()){
        mesh.Render();
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

        for(int c = 0; c < 4; c++){
            switch(cmds[c]){
                case 0x40: {
                        uint32_t mode = stream.readUInt32();
                        currentPrimitive = {};
                        currentPrimitive.SetType(mode);
                    }
                    break;
                
                case 0x41:
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
                    vtx.b = (float)cv5To8(a & 0x1F) / 0xFF;
                    vtx.g = (float)cv5To8((a >> 5) & 0x1F) / 0xFF;
                    vtx.r = (float)cv5To8((a >> 10) & 0x1F) / 0xFF;

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
                    stream.readUInt32();
                    break;
                } 
                case 0x1b: {
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
        return std::move(mesh);
    });

}

}

namespace TEX0 {

Texture::Texture(bStream::CStream& stream, uint32_t texDataOffset){
    std::cout << "Reading Texture at " << std::hex << stream.tell() << std::endl;
    uint32_t params = stream.readUInt32();
    mFormat = (params >> 26) & 0x07;
    mWidth = 8 << ((params >> 20) & 0x07);
    mHeight = 8 << ((params >> 23) & 0x07);
    mColor0 = ((params >> 29) & 0x01);

    mDataOffset = (params & 0xFFFF) << 3;

    size_t pos = stream.tell();
    std::cout << "Reading texture at " << texDataOffset << " + " << mDataOffset << " = " << stream.tell() << std::endl;

    stream.seek(mDataOffset + texDataOffset);
    mImgData.resize(mWidth * mHeight);
    std::fill(mImgData.begin(), mImgData.end(), 0);

    //only support palette16 format for now
    for (size_t y = 0; y < mHeight; y++){
        for(size_t x = 0; x < mWidth; x+=4){
            uint16_t block = stream.readUInt16();
            for(size_t bx = 0; bx < 4; bx++){
                uint16_t paletteIdx = block & 0x0F;
                uint32_t dst = ((y * mWidth) + x + bx);
                mImgData[dst] = paletteIdx;
                block >>= 4;
            } 
        }
    }

    stream.seek(pos);
}

void Texture::Convert(Palette p){
    std::vector<uint8_t> image;
    image.resize(mWidth * mHeight * 4);

    for (size_t y = 0; y < mHeight; y++){
        for(size_t x = 0; x < mWidth; x++){
            uint32_t src = ((y * mWidth) + x);
            uint32_t dst = ((y * mWidth) + x) * 4;
            image[dst]   = p.GetColors()[mImgData[src]].r;
            image[dst+1] = p.GetColors()[mImgData[src]].g;
            image[dst+2] = p.GetColors()[mImgData[src]].b;
            image[dst+3] = mImgData[src] == 0 ? (mColor0 ? 0x00 : 0xFF) : 0xFF; 
        }
    }

    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());

}

void Texture::Bind(){
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE0, mTexture);
}

Texture::~Texture(){
    glDeleteTextures(1, &mTexture);
}

Palette::Palette(bStream::CStream& stream, uint32_t paletteDataOffset, uint32_t paletteDataSize){
    uint32_t paletteOffset = (stream.readUInt16() << 3) + paletteDataOffset;
    stream.skip(2);

    size_t listPos = stream.tell();
    
    size_t tempPos = listPos;
    uint32_t nextPaletteOffset = (stream.peekUInt16(tempPos) << 3) + paletteDataOffset;
    while(nextPaletteOffset == paletteOffset){
        nextPaletteOffset = (stream.peekUInt16(tempPos+=2) << 3) + paletteDataOffset;
    }

    if(nextPaletteOffset > paletteDataOffset + paletteDataSize){
        nextPaletteOffset = paletteDataOffset + paletteDataSize;
    }


    mColorCount = (nextPaletteOffset - paletteOffset) >> 1;
    std::cout << "Current Palette Offset is 0x" << std::hex << paletteOffset << " Next Palette Offset is 0x" << nextPaletteOffset << " color count is " << mColorCount << std::endl; 
    stream.seek(paletteOffset);

    for(int i = 0; i < mColorCount; i++){
        glm::vec3 color;
        uint16_t pentry = stream.readUInt16();
        color.r = cv5To8(pentry & 0x1F);
        color.g = cv5To8((pentry >> 5) & 0x1F);
        color.b = cv5To8((pentry >> 10) & 0x1F);
        mColors.push_back(color);
    }

    stream.seek(listPos);

}

}

void NSBMD::Render(glm::mat4 v){
    if(mReady){
        if(mProgram == 0xFFFFFFFF){
            InitShaders();
        }

        glUseProgram(mProgram);
        glUniformMatrix4fv(glGetUniformLocation(mProgram, "transform"), 1, 0, &v[0][0]);

        for(auto [modelName, model] : mModels.GetItems()){
            mTextures.GetItems()["fsloof"].Bind();
            model.Render();
        }
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
                    return std::move(model);
                });

                break;
            }

            case 0x30584554: { // TEX0 
                stream.readUInt32(); // section size 0x04
                stream.skip(4); //0x08

                uint16_t textureDataSize = stream.readUInt16(); //0x0C
                uint16_t textureListOffset = stream.readUInt16(); //0x0E
                stream.skip(4);
                uint32_t textureDataOffset = stream.readUInt32(); //0x14
                stream.skip(4); //padding //0x18

                uint16_t cmpTexDataSize = stream.readUInt16() << 3; //0x1C
                uint32_t cmpTexInfoOffset = stream.readUInt16(); // 0x1E
                stream.skip(4); // padding 0x20
                
                uint32_t cmpTexDataOffset = stream.readUInt32(); //0x24
                uint32_t cmpTexInfoDataOffset = stream.readUInt32(); // 0x28 huh?
                stream.skip(4); // 0x2C

                uint32_t paletteDataSize = stream.readUInt32() << 3; //30
                uint32_t paletteDictOffset = stream.readUInt16(); //34
                stream.skip(2);
                uint32_t paletteDataOffset = stream.readUInt32(); //

                stream.seek(sectionOffset + paletteDictOffset);
                mPalettes.Load(stream, [&](bStream::CStream& stream){
                    TEX0::Palette palette(stream, paletteDataOffset + sectionOffset, paletteDataSize);
                    return palette;
                });

                stream.seek(sectionOffset + textureListOffset);
                std::cout << "Reading Texture List at " << std::hex << sectionOffset << " " << textureListOffset << std::endl;
                mTextures.Load(stream, [&](bStream::CStream& stream){
                    TEX0::Texture texture(stream, textureDataOffset + sectionOffset);
                    stream.readUInt32(); // wuh?
                    return std::move(texture);
                });

                for(auto [texname, texture] : mTextures.GetItems()){
                    if(mPalettes.GetItems().count(texname) != 0){
                        texture.Convert(mPalettes.GetItems()[texname]);
                    }
                }

                break;   
            }

            default:
                break;
        }
    
    
        stream.seek(returnOffset);

    }

    mReady = true;

}

void NSBMD::Dump(){

}

NSBMD::NSBMD(){

}

NSBMD::~NSBMD(){

}

}