#pragma once
#include "NDS/System/FileSystem.hpp"

namespace Palkia::Nitro {

class Archive
{
private:
    FileSystem mFS;

public:
    void SaveArchive(bStream::CStream& stream);
    size_t GetFileCount() { return mFS.mFiles.size(); }
    std::shared_ptr<File> GetFileByIndex(size_t index);
    void Dump();
    Archive(FileSystem fs) { mFS = fs; }
    Archive(bStream::CStream& stream);
    ~Archive();
};

}