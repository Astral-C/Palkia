#include <filesystem>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <bstream/bstream.h>
#include <NDS/Assets/NSBMD.hpp>
#include <NDS/Assets/NSBTX.hpp>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace py = pybind11;
using namespace py::literals;

static bool init = false;
static glm::mat4 viewMtx = {}, projMtx = {}, camMtx = {};

bool InitPalkia(){
    if(!init){
        if(gladLoadGL()){
            init = true;
            return true;
        }
    }

    return false;
}

void SetCamera(std::vector<float> proj, std::vector<float> view){
    if(init){
        glm::mat4 projection, viewm4;

        projection = glm::make_mat4(proj.data());
        viewm4 = glm::make_mat4(view.data());

        viewMtx = viewm4;
        projMtx = projection;
        camMtx = projection * viewm4;
    }
}

void CleanupPalkia(){
    if(init){
        // cleanup library res
    }
}

std::shared_ptr<Palkia::Formats::NSBMD> LoadNSBMD(std::string path){
    if(!init) return nullptr;

    if(!std::filesystem::exists(path)){
        std::cout << "Couldn't load model " << path << std::endl;
        return nullptr;
    }

    bStream::CFileStream modelStream(path, bStream::Endianess::Big, bStream::OpenMode::In);
    
    std::shared_ptr<Palkia::Formats::NSBMD> modelData = std::make_shared<Palkia::Formats::NSBMD>();
    modelData->Load(modelStream);

    return modelData;
}

std::shared_ptr<Palkia::Formats::NSBMD> LoadNSBMD(py::bytes data){
    if(!init) return nullptr;

    py::buffer_info dataInfo(py::buffer(data).request());

    bStream::CMemoryStream modelStream((uint8_t*)dataInfo.ptr, dataInfo.size, bStream::Endianess::Big, bStream::OpenMode::In);
    
    std::shared_ptr<Palkia::Formats::NSBMD> modelData = std::make_shared<Palkia::Formats::NSBMD>();
    modelData->Load(modelStream);

    return modelData;
}

/*
void setTranslation(std::shared_ptr<J3DModelInstance> instance, float x, float y, float z){
    instance->SetTranslation(glm::vec3(x, y, z));
}

void setRotation(std::shared_ptr<J3DModelInstance> instance, float x, float y, float z){
    instance->SetRotation(glm::vec3(x, y, z));
}

void setScale(std::shared_ptr<J3DModelInstance> instance, float x, float y, float z){
    instance->SetScale(glm::vec3(x, y, z));
}
*/

void renderModel(std::shared_ptr<Palkia::Formats::NSBMD> model){
    model->Render(camMtx);
}

void attachNSBTX(std::shared_ptr<Palkia::Formats::NSBMD> model, std::shared_ptr<Palkia::Formats::NSBTX> texture){
    if(!init) return;
    model->AttachNSBTX(texture.get());
}

/*
    todo: picking?
bool isClicked(std::shared_ptr<J3DModelInstance> instance, uint32_t x, uint32_t y){
}

std::tuple<uint16_t, uint16_t> QueryPicking(uint32_t x, uint32_t y){
}

void InitPicking(uint32_t w, uint32_t h){
}

void ResizePickingFB(uint32_t w, uint32_t h){
}

*/

// still need this? I dunno!!
//void RenderScene(float dt, std::array<float, 3> cameraPos, bool renderPicking = false){
//    if(init){
//        
//    }
//}

PYBIND11_MODULE(J3DUltra, m) {
    m.doc() = "Palkia";

    py::class_<glm::vec3>(m, "Vec3")
        .def(py::init([](){ return glm::vec3(0.0); }))
        .def(py::init([](float x, float y, float z){ return glm::vec3(x,y,z);} ))
        .def_readwrite("x", &glm::vec3::x)
        .def_readwrite("y", &glm::vec3::y)
        .def_readwrite("z", &glm::vec3::z)
        .def_readwrite("r", &glm::vec3::r)
        .def_readwrite("g", &glm::vec3::g)
        .def_readwrite("b", &glm::vec3::b);

    py::class_<glm::vec4>(m, "Vec4")
        .def(py::init([](){ return glm::vec4(0.0); }))
        .def(py::init([](float x, float y, float z, float w){ return glm::vec4(x,y,z,w);} ))
        .def_readwrite("x", &glm::vec4::x)
        .def_readwrite("y", &glm::vec4::y)
        .def_readwrite("z", &glm::vec4::z)
        .def_readwrite("w", &glm::vec4::w)
        .def_readwrite("r", &glm::vec4::r)
        .def_readwrite("g", &glm::vec4::g)
        .def_readwrite("b", &glm::vec4::b)
        .def_readwrite("a", &glm::vec4::a);

    py::class_<Palkia::Formats::NSBTX, std::shared_ptr<Palkia::Formats::NSBTX>>(m, "NSBTX")
        .def(py::init<>());
    // def load NSBTX

    py::class_<Palkia::Formats::NSBMD, std::shared_ptr<Palkia::Formats::NSBMD>>(m, "NSBMD")
        .def(py::init<>())
        .def("render", &renderModel)
        // These don't work yet
        //.def("setTranslation", &setTranslation)
        //.def("setRotation", &setRotation)
        //.def("setScale", &setScale)
        //.def("isClicked", &isClicked)
        .def("attachNSBTX", &attachNSBTX)
    ;
    
    m.def("load", py::overload_cast<std::string>(&LoadNSBMD), "Load NSBMD from filepath", py::kw_only(), py::arg("path"));
    m.def("load", py::overload_cast<py::bytes>(&LoadNSBMD), "Load NSBMD from bytes object", py::kw_only(), py::arg("data"));
   
    
    m.def("init", &InitPalkia, "Setup Palkia for Model Loading and Rendering");
    m.def("cleanup", &CleanupPalkia, "Cleanup Palkia Library");
    m.def("setCamera", &SetCamera, "Set Projection and View Matrices to render with");
    
    //m.def("render", &RenderScene, "Execute all pending model renders");

    //m.def("resizePicking", &ResizePickingFB, "");
    //m.def("queryPicking", &QueryPicking, "");
    //m.def("initPicking", &InitPicking, "");
}