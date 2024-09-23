cmake_minimum_required(VERSION 3.4...3.18)
project(PalkiaPy)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)

# Python version to build for
# set(PYBIND11_PYTHON_VERSION "3.11")

add_subdirectory(pybind11)

pybind11_add_module(PalkiaPy main.cpp)

target_include_directories(PalkiaPy PUBLIC ../include ../lib pybind11/pybind11)

target_link_libraries(PalkiaPy PUBLIC palkia pybind11::embed)

# Build as 'PalkiaPy' but rename to 'Palkia' afterwards to avoid naming conflicts during the build.
set_target_properties(PalkiaPy PROPERTIES OUTPUT_NAME palkia)