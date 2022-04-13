#pragma once
#include "NitroFS.hpp"

class NitroArchive
{
private:
    NitroFS fs;

public:
    NitroFile* getFileByIndex(size_t index);
    void dump();
    NitroArchive(bStream::CStream& stream);
    ~NitroArchive();
};