#include <vector>
#include <glad/glad.h>
#include <NDS/Assets/NSBTX.hpp>
#include <NDS/Assets/NSBMD.hpp>
#include <Util.hpp>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <format>
#include <glm/gtc/matrix_transform.hpp>

namespace Palkia {

const char* default_vtx_shader_source = "#version 460\n\
    #extension GL_ARB_separate_shader_objects : enable\n\
    layout(location = 0) in vec3 inPosition;\n\
    layout(location = 1) in vec3 inNormal;\n\
    layout(location = 2) in vec3 inColor;\n\
    layout(location = 3) in vec2 inTexCoord;\n\
    layout(location = 4) in int inMtxIdx; \n\
    \
    uniform mat4 transform;\n\
    uniform mat4 stackMtx;\n\
    uniform mat4 scaleMtx;\n\
    \
    layout(location = 0) out vec2 fragTexCoord;\n\
    layout(location = 1) out vec3 fragVtxColor;\n\
    \
    void main()\n\
    {\
        gl_Position = transform * stackMtx * scaleMtx * vec4(inPosition, 1.0);\n\
        fragVtxColor = inColor;\n\
        fragTexCoord = inTexCoord;\n\
    }\
";

//uniform int pickID;\n
const char* default_frg_shader_source = "#version 460\n\
    #extension GL_ARB_separate_shader_objects : enable\n\
    \
    uniform sampler2D texSampler;\n\
    uniform mat3x2 texMatrix;\n\
    layout(location = 0) in vec2 fragTexCoord;\n\
    layout(location = 1) in vec3 fragVtxColor;\n\
    \
    layout(location = 0) out vec4 outColor;\n\
    layout(location = 1) out int outPick;\n\
    \
    void main()\n\
    {\n\
        vec4 texel = texture(texSampler, (texMatrix * vec3(fragTexCoord, 0)).xy );\n\
        outColor = texel * vec4(fragVtxColor.xyz, 0.5);\n\
        if(outColor.a < 1.0 / 255.0) discard;\n\
        outPick = 1;\n\
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
    int32_t status;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE){
        int32_t infoLogLength;
        glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &infoLogLength);
        glGetShaderInfoLog(vs, infoLogLength, NULL, glErrorLogBuffer);
        printf("[NSBMD Loader]: Compile failure in mdl vertex shader:\n%s\n", glErrorLogBuffer);
    }
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE){
        int32_t infoLogLength;
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
        int32_t logLen; 
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

void Parse(bStream::CStream& stream, uint32_t offset, Nitro::ResourceDict<std::shared_ptr<MDL0::Model>>& models){
    stream.readUInt32(); // section size
    std::cout << "Reading model list at " << std::hex << stream.tell() << std::endl;
    models = Nitro::ReadList<std::shared_ptr<MDL0::Model>>(stream, [&](bStream::CStream& stream){
        uint32_t modelOffset = stream.readUInt32();
        size_t listPos = stream.tell();

        stream.seek(offset + modelOffset);

        std::shared_ptr<MDL0::Model> model = std::make_shared<MDL0::Model>(stream);

        stream.seek(listPos);
        return model;
    });
}

Primitive::~Primitive(){
    glDeleteVertexArrays(1, &mVao);
    glDeleteBuffers(1, &mVbo);
}

void Primitive::GenerateBuffers(){

    glCreateBuffers(1, &mVbo);
    glNamedBufferStorage(mVbo, sizeof(Vertex)*mVertices.size(), mVertices.data(), GL_DYNAMIC_STORAGE_BIT);

    glCreateVertexArrays(1, &mVao);

    glVertexArrayVertexBuffer(mVao, 0, mVbo, 0, sizeof(Vertex));

    glEnableVertexArrayAttrib(mVao, 0);
    glEnableVertexArrayAttrib(mVao, 1);
    glEnableVertexArrayAttrib(mVao, 2);
    glEnableVertexArrayAttrib(mVao, 3);
    glEnableVertexArrayAttrib(mVao, 4);

    glVertexArrayAttribFormat(mVao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribFormat(mVao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribFormat(mVao, 2, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, color));
    glVertexArrayAttribFormat(mVao, 3, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texcoord));
    glVertexArrayAttribFormat(mVao, 4, 1, GL_UNSIGNED_INT, GL_FALSE, offsetof(Vertex, matrixId));

    glVertexArrayAttribBinding(mVao, 0, 0);
    glVertexArrayAttribBinding(mVao, 1, 0);
    glVertexArrayAttribBinding(mVao, 2, 0);
    glVertexArrayAttribBinding(mVao, 3, 0);
    glVertexArrayAttribBinding(mVao, 4, 0);

}

void Primitive::Render(){
    glBindVertexArray(mVao);

    if(mType == PrimitiveType::Triangles || mType == PrimitiveType::Quads){
        glDrawArrays(GL_TRIANGLES, 0, mVertices.size());
    } else if(mType == PrimitiveType::Tristrips || mType == PrimitiveType::Quadstrips){
        glDrawArrays(GL_TRIANGLE_STRIP, 0, mVertices.size());
    }

    glBindVertexArray(0);
}

void Mesh::Render(){
    for(auto primitive : mPrimitives){
        primitive->Render();
    }
}

void Model::Render(){
    // render based on render commands
    uint32_t curMtxID = 0;
    std::array<glm::mat4, 32> tempMtxStack = mMatrixStack;
    for(auto cmd : mRenderCommands){
        switch(cmd.mOpCode){
            // Load Matrix
            case 0x03:
                curMtxID = cmd.mArgs[0];
                break;
        
            // Bind Material
            case 0x04:
            case 0x24:
            case 0x44:
                mMaterials[cmd.mArgs[0]].second->Bind();
                break;

            // Draw Mesh
            case 0x05:
                glUniformMatrix4fv(glGetUniformLocation(mProgram, "scaleMtx"), 1, 0, &glm::scale(glm::mat4(1.0f), glm::vec3(mUpScale))[0][0]);
                glUniformMatrix4fv(glGetUniformLocation(mProgram, "stackMtx"), 1, 0, &(tempMtxStack[curMtxID])[0][0]);
                mMeshes[cmd.mArgs[0]].second->Render();
                break;

            //mtx stuff
            case 0x06:
            case 0x26:
            case 0x46:
            case 0x66:
                break;
            
            // Calc Skinning
            case 0x09:
                break;

            // Scale up/down
            case 0x0b:
                tempMtxStack[curMtxID] *= mUpScale;
                break;
            case 0x2b:
                tempMtxStack[curMtxID] *= mDownScale;
                break;
            default:
                break;
        }
    }    
}

RenderCommand::RenderCommand(bStream::CStream& stream){
    mOpCode = stream.readUInt8();

    switch(mOpCode){
        // Unknown
        case 0x02:
            mArgs[0] = stream.readUInt8();
            mArgs[1] = stream.readUInt8();
            break;
    
        // Load Matrix
        case 0x03:
            mArgs[0] = stream.readUInt8();
            break;
    
        // Bind Material
        case 0x04:
        case 0x24:
        case 0x44:
            mArgs[0] = stream.readUInt8();
            break;

        // Draw Mesh
        case 0x05:
            mArgs[0] = stream.readUInt8();
            break;

        case 0x06:
        case 0x26:
        case 0x46:
        case 0x66:
            mArgs[0] = stream.readUInt8();
            if(mOpCode == 0x06) break;
            mArgs[1] = stream.readUInt8();
            mArgs[2] = stream.readUInt8();
            mArgs[3] = stream.readUInt8();
            if(mOpCode == 0x26 || mOpCode == 0x46) break;
            mArgs[4] = stream.readUInt8();
            break;
        
        // Unknown
        case 0x07:
        case 0x47:
            mArgs[0] = stream.readUInt8();
            break;

        // Unknown
        case 0x08:
            mArgs[0] = stream.readUInt8();
            break;

        // Calc Skinning
        case 0x09:
            mArgs[0] = stream.readUInt8();
            mArgs[1] = stream.readUInt8(); // # of terms
            for (int i = 0; (i < mArgs[1] && i < 25); i++){
                mArgs[2 + i] = stream.readUInt8();
            }
            break;

        // No Args

        // Nop
        case 0x00:
        // End
        case 0x01:
        // Scale up/down
        case 0x0b:
        case 0x2b:
        // Unknown
        case 0x40:
        case 0x80:
        default:
            break;
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
    std::shared_ptr<Primitive> currentPrimitive = std::make_shared<Primitive>();
    currentPrimitive->SetType(PrimitiveType::None);
    
    struct {
        Vertex vtx {};
    } ctx;

    while(stream.tell() < pos + commandsLen){
        uint8_t cmds[4] = { stream.readUInt8(), stream.readUInt8(), stream.readUInt8(), stream.readUInt8() };

        for(int c = 0; c < 4; c++){
            switch(cmds[c]){
                case 0x40: {
                        uint32_t mode = stream.readUInt32();
                        currentPrimitive = std::make_shared<Primitive>();
                        currentPrimitive->SetType(mode);
                    }
                    break;
                
                case 0x41: {

                    if(currentPrimitive->GetType() == PrimitiveType::Quads){
                        std::vector<Vertex> triangulated;
                        auto verts = currentPrimitive->GetVertices();
                        
                        for(size_t i = 0; i < verts.size(); i+=4){
                            triangulated.push_back(verts[i + 0]);
                            triangulated.push_back(verts[i + 1]);
                            triangulated.push_back(verts[i + 2]);
                            triangulated.push_back(verts[i + 0]);
                            triangulated.push_back(verts[i + 2]);
                            triangulated.push_back(verts[i + 3]);
                        }

                        triangulated.shrink_to_fit();
                        currentPrimitive->SetVertices(triangulated);
                    }

                    currentPrimitive->GenerateBuffers();
                    mPrimitives.push_back(currentPrimitive);
                    break;
                }

                case 0x23: {
                    uint32_t a = stream.readUInt32();
                    uint32_t b = stream.readUInt32();
                    
                    ctx.vtx.position.x = ((((int16_t)(a & 0xFFFF)) << 16) >> 16) / 4096.0f;
                    ctx.vtx.position.y = ((((int16_t)((a >> 16) & 0xFFFF)) << 16) >> 16) / 4096.0f;
                    ctx.vtx.position.z = ((((int16_t)(b & 0xFFFF)) << 16) >> 16) / 4096.0f;

                    currentPrimitive->Push(ctx.vtx);
                    break;
                }

                case 0x24: {
                    uint32_t a = stream.readUInt32();

                    ctx.vtx.position.x = ((((int16_t)((a >>  0) & 0x03FF)) << 22) >> 22) / 64.0f;
                    ctx.vtx.position.y = ((((int16_t)((a >> 10) & 0x03FF)) << 22) >> 22) / 64.0f;
                    ctx.vtx.position.z = ((((int16_t)((a >> 20) & 0x03FF)) << 22) >> 22) / 64.0f;

                    currentPrimitive->Push(ctx.vtx);
                    break;
                }

                case 0x25: {
                    uint32_t a = stream.readUInt32();
                    
                    ctx.vtx.position.x = ((((int16_t)((a >>  0) & 0xFFFF)) << 16) >> 16) / 4096.0f;
                    ctx.vtx.position.y = ((((int16_t)((a >> 16) & 0xFFFF)) << 16) >> 16) / 4096.0f;

                    currentPrimitive->Push(ctx.vtx);
                    break;
                }

                case 0x26: {
                    uint32_t a = stream.readUInt32();

                    ctx.vtx.position.x = ((((int16_t)((a >>  0) & 0xFFFF)) << 16) >> 16) / 4096.0f;
                    ctx.vtx.position.z = ((((int16_t)((a >> 16) & 0xFFFF)) << 16) >> 16) / 4096.0f;
                    
                    currentPrimitive->Push(ctx.vtx);
                    break;
                }

                case 0x27: {
                    uint32_t a = stream.readUInt32();

                    ctx.vtx.position.y = ((((int16_t)((a >>  0) & 0xFFFF)) << 16) >> 16) / 4096.0f;
                    ctx.vtx.position.z = ((((int16_t)((a >> 16) & 0xFFFF)) << 16) >> 16) / 4096.0f;
                    
                    currentPrimitive->Push(ctx.vtx);
                    break;
                }

                case 0x28: {
                    uint32_t a = stream.readUInt32();

                    ctx.vtx.position.x += ((((int16_t)((a >>  0) & 0x03FF)) << 22) >> 22) / 4096.0f;
                    ctx.vtx.position.y += ((((int16_t)((a >> 10) & 0x03FF)) << 22) >> 22) / 4096.0f;
                    ctx.vtx.position.z += ((((int16_t)((a >> 20) & 0x03FF)) << 22) >> 22) / 4096.0f;

                    currentPrimitive->Push(ctx.vtx);
                    break;
                }

                case 0x21: {
                    uint32_t a = stream.readUInt32();

                    glm::vec3 vtx;
                    vtx.x = ((((int16_t)((a >> 0)  & 0x03FF)) << 22) >> 22) / 1024.0f;
                    vtx.y = ((((int16_t)((a >> 10) & 0x03FF)) << 22) >> 22) / 1024.0f;
                    vtx.z = ((((int16_t)((a >> 20) & 0x03FF)) << 22) >> 22) / 1024.0f;

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

                    int16_t s = ((a & 0xFFFF) << 16) >> 16;
                    int16_t t = (((a >> 16) & 0xFFFF) << 16) >> 16;

                    ctx.vtx.texcoord.x = s / 16.0f;
                    ctx.vtx.texcoord.y = t / 16.0f;

                    break;
                }

                case 0x14:{
                    ctx.vtx.matrixId = stream.readUInt32();
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

Material::Material(bStream::CStream& stream){
    stream.readUInt16(); //item tag     0x02
    stream.readUInt16(); // size        0x04
    
    mDiffAmb = stream.readUInt32(); //  0x08
    mSpeEmi = stream.readUInt32();  //  0x0C
    mPolygonAttr = stream.readUInt32(); // 0x10
    uint32_t polygonAttrMask = stream.readUInt32(); // 0x14
    mTexImgParams = stream.readUInt32(); // 0x18
    uint32_t texImgParamsMask = stream.readUInt32(); //0x1C
    uint16_t texturePaletteBase = stream.readUInt16(); 
    uint16_t flag = stream.readUInt16();
    
    uint16_t oWidth = stream.readUInt16();
    uint16_t oHeight = stream.readUInt16();

    int16_t magW = fixed(stream.readUInt32());
    int16_t magH = fixed(stream.readUInt32());


    float scaleU = 1.0f, scaleV = 1.0f, cosR = 1.0f, sinR = 0.0f, transU = 0.0f, transV = 0.0f;
    if(!(flag & 0x0002)){
        scaleU = fixed(stream.readInt32());
        scaleV = fixed(stream.readInt32());
    }
    
    if(!(flag & 0x0004)){
        sinR = fixed(stream.readUInt32());
        cosR = fixed(stream.readUInt32());
    }

    if(!(flag & 0x0008)){
        transU = fixed(stream.readUInt32());
        transV = fixed(stream.readUInt32());
    }

    float texScaleU = 1.0f / (float)oWidth;
    float texScaleV = 1.0f / (float)oHeight;

    mTexMatrix = {
        texScaleU * scaleU * cosR,
        texScaleU * scaleV * -sinR,
        texScaleV * scaleU * sinR,
        texScaleV * scaleV * cosR,
        scaleU * ((-0.5f * cosR) - (0.5 * sinR - 0.5) - transU),
        scaleV * ((-0.5f * cosR) + (0.5 * sinR - 0.5) + transV) + 1.0f
    };

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
    mUpScale = fixed(stream.readUInt32()); // up scale
    mDownScale = fixed(stream.readUInt32()); // down scale

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
    mMeshes = Nitro::ReadList<std::shared_ptr<Mesh>>(stream, [&](bStream::CStream& stream){
        uint32_t offset = stream.readUInt32();
        size_t listPos = stream.tell();

        stream.seek(offset + meshesOffset + modelOffset);

        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(stream);

        stream.seek(listPos);
        return mesh;
    });

    stream.seek(materialsOffset + modelOffset);
    uint16_t materialTextureDictOffset = stream.readUInt16();
    uint16_t materialPaletteDictOffset = stream.readUInt16();

    stream.seek(materialsOffset + modelOffset + materialTextureDictOffset);
    Nitro::ResourceDict<MaterialPair> mMaterialTexturePairs = Nitro::ReadList<MaterialPair>(stream, [&](bStream::CStream& stream){
        MaterialPair p;
        p.mIndexOffset = stream.readUInt16();
        p.mNumMaterials = stream.readUInt8();
        p.mIsBound = stream.readUInt8();
        return p;
    });

    stream.seek(materialsOffset + modelOffset + materialPaletteDictOffset);
    Nitro::ResourceDict<MaterialPair> mMaterialPalettePairs = Nitro::ReadList<MaterialPair>(stream, [&](bStream::CStream& stream){
        MaterialPair p;
        p.mIndexOffset = stream.readUInt16();
        p.mNumMaterials = stream.readUInt8();
        p.mIsBound = stream.readUInt8();
        return p;
    });

    stream.seek(materialsOffset + modelOffset + sizeof(uint16_t) + sizeof(uint16_t));
    std::cout << "Reading material list at " << std::hex << stream.tell() << std::endl;
    mMaterials = Nitro::ReadList<std::shared_ptr<Material>>(stream, [&](bStream::CStream& stream){
        uint32_t offset = stream.readUInt32();
        size_t listPos = stream.tell();

        stream.seek(offset + materialsOffset + modelOffset);

        std::shared_ptr<Material> material = std::make_shared<Material>(stream);

        stream.seek(listPos);
        return material;
    });

    for(size_t i = 0; i < mMaterialTexturePairs.size(); i++){
        for(size_t j = 0; j < mMaterialTexturePairs[i].second.mNumMaterials; j++){
            uint8_t matIdx = stream.peekUInt8(modelOffset + materialsOffset + mMaterialTexturePairs[i].second.mIndexOffset + j);
            mMaterials[matIdx].second->mTextureName = mMaterialTexturePairs[i].first;
        }
    }

    for(size_t i = 0; i < mMaterialPalettePairs.size(); i++){
        for(size_t j = 0; j < mMaterialPalettePairs[i].second.mNumMaterials; j++){
            uint8_t matIdx = stream.peekUInt8(modelOffset + materialsOffset + mMaterialPalettePairs[i].second.mIndexOffset + j);
            mMaterials[matIdx].second->mPaletteName = mMaterialPalettePairs[i].first;
        }
    }

    stream.seek(modelOffset + renderCMDOffset);
    while(stream.tell() < modelOffset + materialsOffset){
        mRenderCommands.push_back(RenderCommand(stream));
        if(mRenderCommands.back().mOpCode == 0x01) break;
    }

    std::fill(mMatrixStack.begin(), mMatrixStack.end(), glm::mat4(1.0f));

    //glCreateBuffers(1, &mUbo);
    //glNamedBufferStorage(mUbo, sizeof(NSBMDUniformBufferObject), &mUniformData GL_DYNAMIC_STORAGE_BIT);
	//glBindBufferRange(GL_UNIFORM_BUFFER, 0, mUbo, 0, sizeof(NSBMDUniformBufferObject));
}

void Material::Bind(){
    glUniformMatrix3x2fv(glGetUniformLocation(mProgram, "texMatrix"), 1, 0, &mTexMatrix[0][0]);
    glBindTextureUnit(0, mTexture);
}

Material::~Material(){
    if(mTexture != 0){
        glDeleteTextures(1, &mTexture);
    }
}

}

namespace Formats {

void NSBMD::Render(glm::mat4 v){
    if(mReady){
        if(mProgram == 0xFFFFFFFF){
            InitShaders();
        }

        glUseProgram(mProgram);
        glUniformMatrix4fv(glGetUniformLocation(mProgram, "transform"), 1, 0, &v[0][0]);

        for(auto [name, model] : mModels.Items()){
            model->Render();
        }
    }
}

void NSBMD::Load(bStream::CStream& stream){
    if(stream.readUInt32() != 0x30444D42) return; // stamp
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
                MDL0::Parse(stream, sectionOffset, mModels);
                break;
            }

            case 0x30584554: { // TEX0 
                TEX0::Parse(stream, sectionOffset, mTextures, mPalettes);
                break;   
            }

            default:
                break;
        }
    
    
        stream.seek(returnOffset);

    }

    // Loop through materials & models to attach textures and palettes to materials - this also handles converting the texture - perhaps convert texture should return gl resource for loaded texture?
    for(auto [name, model] : mModels.Items()){
        for(auto [name, material] : model->GetMaterials().Items()){
            std::cout << "Attaching Texture " << material->mTextureName << " with palette " << material->mPaletteName << std::endl;
            if(mTextures.contains(material->mTextureName) && mPalettes.contains(material->mPaletteName)){
                material->SetTexture(mTextures[material->mTextureName]->Convert(*mPalettes[material->mPaletteName]));
            }
        }
    }

    mReady = true;
}


void NSBMD::AttachNSBTX(NSBTX* nsbtx){
    // Copy materials and palettes from nsbtx to nsbmd
    mTextures = nsbtx->GetTextures();
    mPalettes = nsbtx->GetPalettes();

    // Attach textures same way original model does
    for(auto [name, model] : mModels.Items()){
        for(auto [name, material] : model->GetMaterials().Items()){
            std::cout << "Attaching Texture " << material->mTextureName << " with palette " << material->mPaletteName << std::endl;
            if(mTextures.contains(material->mTextureName) && mPalettes.contains(material->mPaletteName)){
                material->SetTexture(mTextures[material->mTextureName]->Convert(*mPalettes[material->mPaletteName]));
            }
        }
    }
}

}
}