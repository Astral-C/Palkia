cmake_minimum_required(VERSION 3.8)
project(starforge)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -Wall -Wformat-security -Wunused-variable")

project(palkia)

add_compile_definitions(-DGLM_ENABLE_EXPERIMENTAL -DIMGUI_DEFINE_MATH_OPERATORS)


file(GLOB_RECURSE PALKIA_SOURCE
    "include/*.h"
    "include/*.hpp"
    "src/*.cpp"
    "src/*.c"
)

add_library(palkia ${PALKIA_SOURCE})
target_include_directories(palkia PUBLIC include lib lib/glm lib/bstream)
target_link_libraries(palkia PUBLIC GL)

#add_subdirectory(bindings)