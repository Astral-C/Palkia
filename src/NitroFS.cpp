#include "NitroFS.hpp"

NitroFS::NitroFS(){}
NitroFS::~NitroFS(){}

NitroFile* NitroFS::getFileByPath(std::filesystem::path path){
	FSDir d = root;

	for (auto &dir : path){
		if(d.dirs.count(dir) != 0){
			d = d.dirs[dir];
		}
	}


	if(d.files.count(path.filename()) != 0){
		return &files.at(d.files[path.filename()].file_id);
	} else{
		return nullptr;
	}
}

FSDir NitroFS::parseDirectory(bStream::CStream& strm, uint16_t id, std::string path){
	FSDir tempDir;
	tempDir.id = id;
	tempDir.name = path;
	uint32_t dirOffset = strm.peekUInt32(id);
	uint16_t fileId = strm.peekUInt16(id+4);

	while(true){
		uint8_t type = strm.peekUInt8(dirOffset++);

		if(type == 0){
			break;
		}

		bool isDir = (type & 0x80);
		uint8_t nameLen = (type & 0x7F);
		std::string name = strm.peekString(dirOffset, nameLen);
		dirOffset += nameLen;

		if(isDir){
			uint16_t dirId = strm.peekUInt16(dirOffset);
			dirOffset += 2;
			uint16_t subdirOff = (dirId & 0x0FFF) * 0x08;
			tempDir.dirs.insert({name, parseDirectory(strm, subdirOff, name)});

		} else {
			tempDir.files.insert({name, {fileId, fileId, name}});
			fileId++;
		}
	}
	return tempDir;
}


void NitroFS::parseRoot(bStream::CStream& strm, size_t fnt_offset, size_t fnt_size, size_t fat_offset, size_t fat_size){
	bStream::CMemoryStream fntBuffer = bStream::CMemoryStream(fnt_size, bStream::Endianess::Little, bStream::OpenMode::In);
	strm.seek(fnt_offset, false);
	strm.readBytesTo(fntBuffer.getBuffer(), fnt_size);
	
	strm.seek(fat_offset, false);

	for (size_t f = 0; f < int(fat_size / 8); f++){
		uint32_t start = strm.readUInt32();
		uint32_t end = strm.readUInt32();

		uint8_t* file = new uint8_t[end - start];
		size_t r = strm.tell();
		strm.seek(start, false);
		strm.readBytesTo(file, end - start);
		strm.seek(r, false);

		files.push_back({end - start, file});
	}

	root = parseDirectory(fntBuffer, 0, "");
}