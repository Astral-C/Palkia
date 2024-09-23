#pragma once
#include <Util.hpp>
#include <bstream/bstream.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <NDS/Assets/NSBTX.hpp>

namespace Palkia {

class NSBMD;

namespace MDL0 {
    
    typedef enum {
        Triangles,
        Quads,
        Tristrips,
        Quadstrips,
        None
    } PrimitiveType;

    struct Vertex {
        glm::vec3 position { 0.0f, 0.0f, 0.0f };
        glm::vec3 normal { 0.0f, 0.0f, 0.0f };
        glm::vec3 color { 1.0f, 1.0f, 1.0f };
        glm::vec2 texcoord { 0.0f, 0.0f };
        uint32_t matrixId { 0 };
    };


    class Mesh;
    class Primitive {
    friend Mesh;
        uint32_t mVao, mVbo;
        PrimitiveType mType;
        std::vector<Vertex> mVertices {};
    public:
        void Push(Vertex v) { mVertices.push_back(v); }

        void GenerateBuffers();

        void SetType(uint32_t t) { mType = (PrimitiveType)(t); }
        PrimitiveType GetType() { return mType; }
        std::vector<Vertex>& GetVertices() { return mVertices; }
        void SetVertices(std::vector<Vertex>& v) { mVertices = v; }

        void Render();

        Primitive(){}
        ~Primitive();
    };

    class Bone { };
    
    struct RenderCommand { 
        uint8_t mOpCode { 1 };
        uint8_t mArgs[25] { 0 };

        RenderCommand(){}
        RenderCommand(bStream::CStream& stream);
        ~RenderCommand(){}
    };

    struct MaterialPair {
        uint16_t mIndexOffset;
        uint8_t mNumMaterials;
        uint8_t mIsBound;
    };

    class Material { 
        uint32_t mDiffAmb;
        uint32_t mSpeEmi;
        uint32_t mPolygonAttr;
        uint32_t mTexImgParams; // texwidth/height are duplicates?
        glm::mat3x2 mTexMatrix;
        uint32_t mTexture { 0 };

    public:
        std::string mTextureName, mPaletteName;

        void SetTexture(uint32_t t) { mTexture = t; }

        void Bind();

        Material(){}
        Material(bStream::CStream&);
        ~Material();
    };

    class Mesh {
        std::vector<std::shared_ptr<Primitive>> mPrimitives {};
    public:
        std::vector<std::shared_ptr<Primitive>>& GetPrimitives() { return mPrimitives; }

        void Render();

        Mesh(){}
        Mesh(bStream::CStream& stream);
        ~Mesh(){}
    };

    class Model { // MDL0
        uint32_t mUbo { 0 };
        std::vector<Bone> mBones;
        std::vector<RenderCommand> mRenderCommands;
        Nitro::ResourceDict<std::shared_ptr<Mesh>> mMeshes;
        Nitro::ResourceDict<std::shared_ptr<Material>> mMaterials;
        std::array<glm::mat4, 32> mMatrixStack;
        float mUpScale { 1.0f };
        float mDownScale { 1.0f };

    public:

        Nitro::ResourceDict<std::shared_ptr<Mesh>>& GetMeshes() { return mMeshes; }
        Nitro::ResourceDict<std::shared_ptr<Material>>& GetMaterials() { return mMaterials; }

        void Dump();
        void Render();

        Model(){}
        Model(bStream::CStream& stream);
        ~Model(){}
    };

    void Parse(bStream::CStream& stream, uint32_t offset, Nitro::ResourceDict<std::shared_ptr<MDL0::Model>>& models);
}

namespace Formats {

class NSBMD {
    bool mReady { false };
    Nitro::ResourceDict<std::shared_ptr<MDL0::Model>> mModels;
    Nitro::ResourceDict<std::shared_ptr<TEX0::Texture>> mTextures;
    Nitro::ResourceDict<std::shared_ptr<TEX0::Palette>> mPalettes;
    
public:
    void AttachNSBTX(NSBTX* nsbtx);

    void Render(glm::mat4 v);
    void Load(bStream::CStream& stream);

    NSBMD(){}
    ~NSBMD(){}
};

}
}