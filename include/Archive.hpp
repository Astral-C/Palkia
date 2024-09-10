#pragma once
#include "FileSystem.hpp"

namespace Palkia::Nitro {

class Archive
{
private:
    FileSystem mFS;

public:
    File* GetFileByIndex(size_t index);
    void Dump();
    Archive(bStream::CStream& stream, bool hasFNT = true);
    ~Archive();
};

}