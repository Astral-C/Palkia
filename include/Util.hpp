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

template<typename T>
float fixed(T n){
    return (float)n / (1 << 12);
}

namespace Nitro {

typedef char Name[16];

template <typename T>
class List {
    size_t mSize { 0 };
    Name* mNames { nullptr };
    T* mItems { nullptr };

public:

    class iterator {
        T* ptr;
    public:
        using iterator_concept  = std::contiguous_iterator_tag;
        using iterator_category = std::contiguous_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        iterator(T* p=nullptr) : ptr { p } {}

        iterator& operator++(){
            ptr++;
            return *this;
        }

        iterator operator++(int){
            auto ret = *this;
            ptr++;
            return ret;
        }

        iterator& operator--(){
            ptr--;
            return *this;
        }

        T& operator[](const size_t index) const {
            return *(ptr + index);
        }

        const bool operator==(const iterator& o) const {
            return ptr == o.ptr;
        }

        T& operator*() const {
            return *ptr;
        }

        iterator operator+(const size_t n) const {
            return iterator(ptr + n);
        }

        friend const iterator operator+(const size_t n, const iterator& o){
            return iterator(o.ptr + n);
        }

        const iterator operator-(const size_t n){
            return iterator(ptr - n);
        }

        const size_t operator-(const iterator& o){
            return ptr - o.ptr;
        }

        iterator& operator+=(const size_t n) {
            ptr += n;
            return *this;
        }

        iterator& operator-=(const size_t n) {
            ptr -= n;
            return *this;
        }
    };

    iterator begin() const { return iterator(mItems); }
    iterator end() const { return iterator(mItems + mSize); }

    List(){}

    void Load(bStream::CStream& stream, std::function<T(bStream::CStream&)> read){
        stream.skip(1); // dummy
        mSize = stream.readInt8();

        mItems = new T[mSize];
        mNames = new Name[mSize];

        uint16_t listSize = stream.readUInt16();

        stream.skip(8 + (4 * mSize)); // undocumented

        stream.readUInt16(); // size of list item in bytes
        stream.readUInt16(); // size of data section

        for (size_t i = 0; i < mSize; i++){
            mItems[i] = read(stream);
            std::string name = stream.readString(16);
            strncpy(mNames[i], name.c_str(), sizeof(mNames[i]));
        }
    }

    List(const List& other){
        mSize = other.mSize;
        mItems = new T[mSize];
        mNames = new Name[mSize];

        for(size_t i = 0; i < mSize; i++){
            mItems[i] = other.mItems[i];
            strncpy(mNames[i], other.mNames[i], sizeof(mNames[i]));
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