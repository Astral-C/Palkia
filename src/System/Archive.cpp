#include "System/Archive.hpp"
#include "Util.hpp"

namespace Palkia::Nitro {

void Archive::Dump(){
    mFS.GetRoot()->Dump(".");
}

Archive::Archive(bStream::CStream& stream){
    //size_t fnt_offset = stream.peekUInt32(0x14) + 0x14;
    //size_t fnt_size = stream.peekUInt32(fnt_offset); //includes header
    //mFS.ParseRoot(stream, fnt_offset + 0x04, fnt_size - 0x08, 0x1C, stream.peekUInt32(0x14), fnt_offset + fnt_size + 0x04);

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

}


Archive::~Archive(){}

}