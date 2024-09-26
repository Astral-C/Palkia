#include "NDS/System/Archive.hpp"
#include "Util.hpp"

namespace Palkia::Nitro {

void Archive::Dump(){
    if(mFS.mHasFNT){
        mFS.GetRoot()->Dump(".");
    } else {
        std::filesystem::create_directory("archive");
        for (auto& file : mFS.mFiles){
            bStream::CFileStream out(std::filesystem::path("archive") / file->GetName(), bStream::OpenMode::Out);
            out.writeBytes(file->GetData(), file->GetSize());
        }
    }
}

std::shared_ptr<File> Archive::GetFileByIndex(size_t index){

    if(!mFS.mHasFNT){
        return mFS.mFiles[index];
    } else {
        std::shared_ptr<File> r = nullptr;
        mFS.ForEachFile([&](std::shared_ptr<File> f){
            if(f->GetID() == index){
                return;
            }
        });
        return r;
    }

}

Archive::Archive(bStream::CStream& stream){

	stream.seek(0x10);
    stream.readUInt32(); // BTAF
    uint32_t fatSize = stream.readUInt32(); // section size
    uint32_t fileCount = stream.readUInt32(); // file count
    size_t fatOffset = stream.tell();

    stream.seek(fatSize + 0x10);
    stream.readUInt32(); // BTNF
    uint32_t fntSize = stream.readUInt32(); // section size
    size_t fntOffset = stream.tell();

    stream.seek(fatSize + fntSize + 0x10);

    stream.readUInt32(); // GMIF
    stream.readUInt32(); // section size
    uint32_t imgOffset = stream.tell();

    stream.seek(fatOffset);
	std::vector<std::shared_ptr<File>> files;
    int id = 0;
	for(auto file : mFS.ParseFAT(stream, fileCount)){
		files.push_back(File::Load(stream, id++, file.first + imgOffset, file.second + imgOffset));
	}

    stream.seek(fntOffset);
	mFS.mRoot = mFS.ParseFNT(stream, fntSize - 0x08, files);

    if(mFS.mRoot == nullptr){ // no FNT!
        mFS.mHasFNT = false;
        mFS.mFiles = std::move(files);
    }

}


Archive::~Archive(){}

}