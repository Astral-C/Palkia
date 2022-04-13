#pragma once
#include "NitroFS.hpp"

class NitroArchive
{
private:
    NitroFS fs;

public:
    NitroFile* getFileByIndex(size_t index);
    NitroArchive(bStream::CStream& stream);
    ~NitroArchive();
};