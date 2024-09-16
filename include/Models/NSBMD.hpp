#pragma once
#include <Util.hpp>
#include <bstream/bstream.h>
#include <glm/glm.hpp>
#include <vector>

namespace Palkia {

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

    struct faceVtx {
        uint32_t posIdx;
        uint32_t normalIdx;
        uint32_t texcoordIdx;
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
    Nitro::List<MDL0::Model> mModels;
        
public:
    void Dump();
    void Render(glm::mat4 v);
    void Load(bStream::CStream& stream);
    NSBMD();
    ~NSBMD();
};

}