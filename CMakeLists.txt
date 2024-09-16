cmake_minimum_required(VERSION 3.8)
project(starforge)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -Wall -Wformat-security")

project(palkia)

add_compile_definitions(-DGLM_ENABLE_EXPERIMENTAL -DIMGUI_DEFINE_MATH_OPERATORS)

include_directories(lib/imgui) # required due to imgui file dialog being a bit finnicky
add_subdirectory(lib)


file(GLOB_RECURSE PALKIA_SOURCE
    "include/*.h"
    "include/*.hpp"
    "src/*.cpp"
    "src/*.c"

    "lib/ImGuiFileDialog/ImGuiFileDialog/*.h"
    "lib/imgui/misc/cpp/*.cpp"
)

add_executable(palkia ${PALKIA_SOURCE})
target_include_directories(palkia PUBLIC include lib lib/bstream lib/ImGuiFileDialog/ImGuiFileDialog  lib/glfw/include)
target_link_libraries(palkia PUBLIC GL glfw imgui ImGuiFileDialog)