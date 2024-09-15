#pragma once
#include <Util.hpp>
#include <bstream/bstream.h>
#include <glm/glm.hpp>
#include <vector>

namespace Palkia {

namespace MDL0 {
    
    class Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec4 color;
    };

    class RenderCommand { };
    class Material { };
    class Bone { };

    class Mesh {
        std::vector<Vertex> mVertices {};
    public:
        Mesh(){}
        Mesh(bStream::CStream& stream);
2        ~Mesh(){}
    };

    class Model { // MDL0
        Nitro::List<RenderCommand> mRenderCommands;
        Nitro::List<Material> mMaterials;
        Nitro::List<Mesh> mMeshes;
        Nitro::List<Bone> mBones;

    public:
        Model(){}
        Model(bStream::CStream& stream);
        ~Model(){}
    };

}

class NSBMD {
    Nitro::List<MDL0::Model> mModels;
        
public:
    void Load(bStream::CStream& stream);
    NSBMD();
    ~NSBMD();
};

}