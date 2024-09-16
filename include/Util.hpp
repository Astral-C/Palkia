#pragma once
#include <cstdint>
#include <bstream/bstream.h>
#include <functional>
#include <vector>
#include <array>

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
class List {
    size_t mSize { 0 };
    std::vector<std::array<char, 16>> mNames { };
    std::vector<T> mItems { };

public:

    size_t size() { return mSize; }

    std::vector<T>& GetItems() { return mItems; }
    std::vector<std::array<char, 16>>& GetNames() { return mNames; }

    std::vector<T>::iterator begin() const { return mItems.begin(); }
    std::vector<T>::iterator end() const { return mItems.end(); }

    List(){}

    void Load(bStream::CStream& stream, std::function<T(bStream::CStream&)> read){
        stream.skip(1); // dummy
        mSize = stream.readInt8();

        mItems.resize(mSize);
        mNames.resize(mSize);

        stream.readUInt16(); // list size

        stream.skip(8 + (4 * mSize)); // undocumented

        stream.readUInt16(); // size of list item in bytes
        stream.readUInt16(); // size of data section

        for (size_t i = 0; i < mSize; i++){
            mItems[i] = read(stream);
        }
        for (size_t i = 0; i < mSize; i++){
            std::string name = stream.readString(16);
            strncpy(mNames[i].data(), name.c_str(), sizeof(mNames[i]));
        }
    }

    List(const List& other){
        mSize = other.mSize;
        mItems = other.mItems;
        mNames = other.mNames;
    }

    ~List(){}
};

}

}