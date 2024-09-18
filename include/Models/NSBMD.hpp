#pragma once
#include <Util.hpp>
#include <bstream/bstream.h>
#include <glm/glm.hpp>
#include <vector>


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

        std::vector<uint16_t> mImgData;
        uint32_t mTexture { 0 };


    public:
    
        uint32_t GetFormat() { return mFormat; }
        uint32_t GetWidth() { return mWidth; }
        uint32_t GetHeight() { return mHeight; }
        Texture(bStream::CStream&, uint32_t);

        void Convert(Palette p);
        
        void Bind();

        Texture(){}
        ~Texture();

    };

};

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
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 texcoord;
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

        void Render();

        Primitive(){}
        ~Primitive(){}
    };

    class RenderCommand { };
    class Material { };
    class Bone { };


    class Mesh {
        std::vector<Primitive> mPrimitives {};
    public:
        std::vector<Primitive>& GetPrimitives() { return mPrimitives; }

        void Render();

        Mesh(){}
        Mesh(bStream::CStream& stream);
        ~Mesh(){}
    };

    class Model { // MDL0
        Nitro::List<RenderCommand> mRenderCommands;
        Nitro::List<Material> mMaterials;
        Nitro::List<Mesh> mMeshes;
        Nitro::List<Bone> mBones;

    public:

        Nitro::List<Mesh>& GetMeshes() { return mMeshes; }

        void Dump();
        void Render();

        Model(){}
        Model(bStream::CStream& stream);
        ~Model(){}
    };

}

class NSBMD {
    bool mReady { false };
    Nitro::List<MDL0::Model> mModels;
    Nitro::List<TEX0::Texture> mTextures;
    Nitro::List<TEX0::Palette> mPalettes;
        
public:
    void Dump();
    void Render(glm::mat4 v);
    void Load(bStream::CStream& stream);
    NSBMD();
    ~NSBMD();
};

}