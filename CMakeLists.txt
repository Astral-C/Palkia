cmake_minimum_required(VERSION 3.8)

#oof
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fpermissive -Wall -Wformat-security")

project(palkia)

#our include dir
include_directories(include)
file(GLOB_RECURSE LIBRARY_SOURCE
    "bstream/*.h"
    "bstream/*.cpp"
    "include/*.h"
    "src/*.cpp"
)

add_library(palkia STATIC ${LIBRARY_SOURCE})