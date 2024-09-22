#pragma once
#include <Util.hpp>
#include <bstream/bstream.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace Palkia {

namespace TEX0 {
    class Palette { 
        uint32_t mColorCount { 0 };
        std::vector<glm::vec3> mColors;
    public: 
        std::vector<glm::vec3> GetColors() { return mColors; }
        Palette(bStream::CStream&, uint32_t, uint32_t);
        Palette(){}
        ~Palette(){}
    };

    class Texture {
        uint32_t mFormat;
        uint32_t mWidth, mHeight;
        uint32_t mColor0;
        uint32_t mDataOffset;

        std::vector<uint32_t> mImgData;


    public:
    
        uint32_t GetFormat() { return mFormat; }
        uint32_t GetWidth() { return mWidth; }
        uint32_t GetHeight() { return mHeight; }
        Texture(bStream::CStream&, uint32_t);

        uint32_t Convert(Palette p);
        
        void Bind();

        Texture(){}
        ~Texture(){}

    };

};

class NSBMD;

namespace MDL0 {
    class Mesh;
    
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
    };

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

    class RenderCommand { };
    class Bone { };

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

        void Bind();

        Material(){}
        Material(bStream::CStream&);
        ~Material(){
            if(mTexture != 0){
                glDeleteTextures(1, &mTexture);
            }
        }
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
        Nitro::ResourceDict<std::shared_ptr<RenderCommand>> mRenderCommands;
        Nitro::ResourceDict<std::shared_ptr<Material>> mMaterials;
        Nitro::ResourceDict<std::shared_ptr<Mesh>> mMeshes;
        Nitro::ResourceDict<std::shared_ptr<Bone>> mBones;

    public:

        Nitro::ResourceDict<std::shared_ptr<Mesh>>& GetMeshes() { return mMeshes; }

        void Dump();
        void Render();

        void AttachMaterials(NSBMD* nsbmd);

        Model(){}
        Model(bStream::CStream& stream);
        ~Model(){}
    };

}

class NSBMD {
    bool mReady { false };
    Nitro::ResourceDict<std::shared_ptr<MDL0::Model>> mModels;
    Nitro::ResourceDict<std::shared_ptr<TEX0::Texture>> mTextures;
    Nitro::ResourceDict<std::shared_ptr<TEX0::Palette>> mPalettes;
        
public:
    void Dump();
    void Render(glm::mat4 v);
    void Load(bStream::CStream& stream);
    NSBMD();
    ~NSBMD();
};

}