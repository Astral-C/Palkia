#include "NDS/System/Archive.hpp"
#include "Util.hpp"
#include <algorithm>

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

void Archive::SaveArchive(bStream::CStream& stream){
    uint32_t fntSize = mFS.CalculateFNTSize();
    uint32_t fatSize = mFS.CalculateFATSize();
    uint32_t imgSize = 0;

    mFS.ForEachFile([&](std::shared_ptr<File> f) { imgSize += PadTo32(f->GetSize()); });

    fntSize = PadTo32(fntSize);
    fatSize = PadTo32(fatSize);
    imgSize = PadTo32(imgSize);

    uint32_t archiveSize = 0x10 + 0x18 + fntSize + fatSize + imgSize;

    uint8_t* fntData = new uint8_t[fntSize];
    uint8_t* fatData = new uint8_t[fatSize];
    uint8_t* imgData = new uint8_t[imgSize];

    memset(fntData, 0, fntSize);
    memset(fatData, 0, fatSize);
    memset(imgData, 0, imgSize);

    bStream::CMemoryStream fntStream(fntData, fntSize, bStream::Endianess::Little, bStream::OpenMode::Out);
    bStream::CMemoryStream fatStream(fatData, fatSize, bStream::Endianess::Little, bStream::OpenMode::Out);
    bStream::CMemoryStream imgStream(imgData, imgSize, bStream::Endianess::Little, bStream::OpenMode::Out);

    // calling write FNT will reset all IDs! it MUST be called FIRST
    mFS.WriteFNT(fntStream);
    mFS.WriteFAT(fatStream);

	std::vector<std::shared_ptr<File>> files = {};

	mFS.ForEachFile([&](std::shared_ptr<File> f){
		files.push_back(f);
	});
	
	std::sort(files.begin(), files.end(), [](std::shared_ptr<File> a, std::shared_ptr<File> b){ return a->GetID() < b->GetID(); });
    
    for(std::size_t i = 0; i < files.size(); i++){
        imgStream.writeBytes(files[i]->GetData(), files[i]->GetSize());
        uint32_t paddedSize = PadTo32(imgStream.tell()); 
        while(imgStream.tell() < paddedSize) imgStream.writeUInt8(0);
    }

    // Write NARC header
    stream.writeUInt32(0x4352414E);
    stream.writeUInt32(0x0100FFFE);
    stream.writeUInt32(archiveSize);
    stream.writeUInt16(0x10);
    stream.writeUInt16(3);

    // Write FAT header
    stream.writeUInt32(0x46415442);
    stream.writeUInt32(fatSize + 0x0C);
    stream.writeUInt32(files.size());
    stream.writeBytes(fatData, fatSize);

    // Write FNT header
    stream.writeUInt32(0x464E5442);
    stream.writeUInt32(fntSize + 0x08);
    stream.writeBytes(fntData, fntSize);

    // Write GMIF header
    stream.writeUInt32(0x46494D47);
    stream.writeUInt32(imgSize);
    stream.writeBytes(imgData, imgSize);

    delete[] fntData;
    delete[] fatData;
    delete[] imgData;
}

Archive::Archive(bStream::CStream& stream){
	stream.seek(0x10);
    stream.readUInt32(); // BTAF
    uint32_t fatSize = stream.readUInt32(); // section size 0x00
    uint32_t fileCount = stream.readUInt32(); // file count 0x08
    size_t fatOffset = stream.tell();

    stream.seek(fatSize + 0x10);
    stream.readUInt32(); // BTNF 0x0C
    uint32_t fntSize = stream.readUInt32(); // section size //0x10
    size_t fntOffset = stream.tell();

    stream.seek(fatSize + fntSize + 0x10);

    stream.readUInt32(); // GMIF 0x14
    stream.readUInt32(); // section size 0x18
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