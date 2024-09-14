#pragma once
#include "System/FileSystem.hpp"

namespace Palkia::Nitro {

class Archive
{
private:
    FileSystem mFS;

public:
    size_t GetFileCount();
    std::shared_ptr<File> GetFileByIndex(size_t index);
    void Dump();
    Archive(bStream::CStream& stream);
    ~Archive();
};

}