#pragma once
#include <cstdint>
#include <bstream/bstream.h>
#include <functional>

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

namespace Nitro {

typedef char Name[16];

template <typename T>
class List {
    size_t mItemCount { 0 };
    Name* mNames { nullptr };
    T* mItems { nullptr };

public:
    List(int size){
        //mItems = new T[size];
        //mNames = new std::string[size];
    }

    List(bStream::CStream& stream, std::function<T(bStream::CStream&)> read){
        stream.skip(1); // dummy
        uint8_t count = stream.readInt8();

        mItems = new T[count];
        mNames = new Name[count];

        uint16_t listSize = stream.readUInt16();

        stream.skip(8 + (4 * count)); // undocumented

        stream.readUInt16(); // size of list item in bytes
        stream.readUInt16(); // size of data section

        for (size_t i = 0; i < count; i++){
            mItems[i] = read(stream);
            std::string name = stream.readString(16);
            strncpy(mNames[i], name.c_str(), sizeof(mNames[i]));
        }
    }

    ~List(){
        if(mItems != nullptr){
            delete[] mItems;
        }

        if(mNames != nullptr){
            delete[] mNames;
        }
    }
};

}

}