#include "NitroArchive.hpp"

namespace Palkia {

void DumpDir(FSDir* dir, NitroFS* fs, std::filesystem::path out_path){
    for (auto& dir : dir->dirs){
        std::filesystem::create_directory(std::filesystem::path("dump") / out_path / dir.first);    
        Palkia::DumpDir(&dir.second, fs, out_path / dir.first);
    }

    for (auto& file : dir->files){
        bStream::CFileStream out(std::filesystem::path("dump") / out_path / file.first, bStream::OpenMode::Out);
        NitroFile* f = fs->getFileByIndex(file.second);
        if(f != nullptr){
            out.writeBytes((char*)f->data, f->size);
        } else {
            std::cout << "couldn't get file " << out_path / file.first << std::endl;
        }
    }
    
}

void NitroArchive::dump(){
    std::filesystem::create_directory("dump");
    Palkia::DumpDir(fs.getRoot(), &fs, "");
}


NitroArchive::NitroArchive(bStream::CStream& stream, bool hasFNT){

    size_t fnt_offset = stream.peekUInt32(0x14) + 0x14;
    size_t fnt_size = stream.peekUInt32(fnt_offset); //includes header
    fs.parseRoot(stream, fnt_offset + 0x04, fnt_size - 0x08, 0x1C, stream.peekUInt32(0x14), fnt_offset + fnt_size + 0x04,  hasFNT);
}

NitroFile* NitroArchive::getFileByIndex(size_t index){
    return fs.getFileByIndex(index);
}

NitroArchive::~NitroArchive(){
}

}