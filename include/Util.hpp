#ifndef __UTIL_H__
#define __UTIL_H__
#include <cstdint>
#include "bstream/bstream.h"
#include <functional>
#include <vector>
#include <unordered_map>

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

uint8_t cv3To8(uint8_t v);
uint8_t cv5To8(uint8_t v);
uint8_t s3tcBlend(uint8_t a, uint8_t b);
uint32_t PadTo32(uint32_t x);
uint32_t Pad(uint32_t x, uint32_t y);

namespace Nitro {

template <typename T>
class ResourceDict {
    std::vector<std::pair<std::string, T>> mItems;

public:
    std::vector<std::pair<std::string, T>>& Items(){
        return mItems;
    }

    T& operator[](std::string& key){
        for(size_t i = 0; i < mItems.size(); i++){
            if(mItems[i].first == key){
                return mItems[i].second; 
            }
        }
        mItems.push_back({key, T()});
        return mItems.back().second;
    }

    std::pair<std::string, T>& operator[](int idx){
        return mItems[idx];
    }

    bool contains(std::string& key){
        for(auto item : mItems){
            if(item.first == key){
                return true; 
            }
        }
        return false;
    }

    size_t size(){
        return mItems.size();
    }

    void resize(size_t size){
        mItems.resize(size);
    }



    ResourceDict(){}
    ~ResourceDict(){}
};

template <typename T>
ResourceDict<T> ReadList(bStream::CStream& stream, std::function<T(bStream::CStream&)> read){
    size_t size = 0;
    stream.skip(1); // dummy

    size = stream.readInt8();

    ResourceDict<T> items;

    items.resize(size);
    stream.readUInt16(); // list size

    stream.skip(8 + (4 * size)); // undocumented

    stream.readUInt16(); // size of list item in bytes
    stream.readUInt16(); // size of data section

    for (size_t i = 0; i < size; i++){
        items[i] = {"dummy", read(stream)};
    }
    for (size_t i = 0; i < size; i++){
        std::string name = stream.readString(16);
        items[i].first = name;
    }

    return items;
}

}

}

#endif