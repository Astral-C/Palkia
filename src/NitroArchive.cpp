#include "NitroArchive.hpp"

namespace Palkia {

NitroArchive::NitroArchive(bStream::CStream& stream){
    size_t fnt_offset = stream.peekUInt32(0x14) + 0x10;
    size_t fnt_size = stream.peekUInt32(fnt_offset + 0x04); //includes header
    stream.seek(0x20);
    fs.parseRoot(stream, fnt_offset, fnt_size, 0x20, stream.peekUInt32(0x14), fnt_offset + fnt_size + 0x10, fnt_size - 0x10 != 0);
}

NitroFile* NitroArchive::getFileByIndex(size_t index){
    return fs.getFileByIndex(index);
}

NitroArchive::~NitroArchive(){
}

}