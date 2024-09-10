#include "Archive.hpp"

namespace Palkia::Nitro {

void DumpDir(Directory* dir, FileSystem* mFS, std::filesystem::path out_path){
    for (auto& dir : dir->mDirectories){
        std::filesystem::create_directory(out_path / dir.first);
        DumpDir(&dir.second, mFS, out_path / dir.first);
    }

    for (auto& file : dir->mFiles){
        bStream::CFileStream out(out_path / file.first, bStream::OpenMode::Out);
        File* f = mFS->GetFileByIndex(file.second);
        if(f != nullptr){
            out.writeBytes((char*)f->data, f->size);
        } else {
            std::cout << "Couldn't get file " << out_path / file.first << std::endl;
        }
    }
    
}

void Archive::Dump(){
    DumpDir(mFS.GetRoot(), &mFS, "");
}


Archive::Archive(bStream::CStream& stream, bool hasFNT){
    size_t fnt_ofmFSet = stream.peekUInt32(0x14) + 0x14;
    size_t fnt_size = stream.peekUInt32(fnt_ofmFSet); //includes header
    mFS.ParseRoot(stream, fnt_ofmFSet + 0x04, fnt_size - 0x08, 0x1C, stream.peekUInt32(0x14), fnt_ofmFSet + fnt_size + 0x04,  hasFNT);
}

File* Archive::GetFileByIndex(size_t index){
    return mFS.GetFileByIndex(index);
}

Archive::~Archive(){}

}