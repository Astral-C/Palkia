#pragma once
#include "System/FileSystem.hpp"

namespace Palkia::Nitro {

class Archive
{
private:
    FileSystem mFS;

public:
    size_t GetFileCount();
    File* GetFileByIndex(size_t index);
    void Dump();
    Archive(bStream::CStream& stream);
    ~Archive();
};

}