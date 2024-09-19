#pragma once
#include <cstdint>
#include <bstream/bstream.h>
#include <functional>
#include <vector>
#include <map>

namespace Palkia {


typedef union {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
    uint32_t rgba; 
} Color;

template<typename T>
float fixed(T n){
    return (float)n / (1 << 12);
}

namespace Nitro {

template <typename T>
std::map<std::string, T> ReadList(bStream::CStream& stream, std::function<T(bStream::CStream&)> read){
    size_t size = 0;
    stream.skip(1); // dummy

    size = stream.readInt8();

    std::vector<T> tempItems;
    std::map<std::string, T> items;

    tempItems.resize(size);
    stream.readUInt16(); // list size

    stream.skip(8 + (4 * size)); // undocumented

    stream.readUInt16(); // size of list item in bytes
    stream.readUInt16(); // size of data section

    for (size_t i = 0; i < size; i++){
        tempItems[i] = read(stream);
    }
    for (size_t i = 0; i < size; i++){
        std::string name = stream.readString(16);
        items[name] = tempItems[i];
    }

    return items;
}

}

}