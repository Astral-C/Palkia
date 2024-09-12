#include "Util.hpp"
#include "System/FileSystem.hpp"

namespace Palkia::Nitro {

void Folder::Dump(std::filesystem::path out_path){
    for (auto& directory : mFolders){
        std::filesystem::create_directory(out_path / directory->GetName());
        directory->Dump(out_path / directory->GetName());
    }

    for (auto& file : mFiles){
		std::cout << "Dumping file " << file->GetName() << std::endl; 
        bStream::CFileStream out(out_path / file->GetName(), bStream::OpenMode::Out);
        out.writeBytes(file->GetData(), file->GetSize());
    }
    
}

FileSystem::FileSystem(){}
FileSystem::~FileSystem(){}

std::shared_ptr<File> Folder::GetFile(std::filesystem::path path){
    if(path.begin() == path.end()) return nullptr;

    for(auto file : mFiles){
        if(file->GetName() == path.begin()->string()){
            if(((++path.begin()) == path.end())){
                return file;
            }
        }
    }

    std::shared_ptr<File> file = nullptr;

    for(auto dir : mFolders){
        if(dir->GetName() == path.begin()->string()){
            std::filesystem::path subPath;
            for(auto it = (++path.begin()); it != path.end(); it++) subPath  = subPath / it->string();
            file = dir->GetFile(subPath);
        }
    }

    return file;
}

std::shared_ptr<File> FileSystem::GetFile(std::filesystem::path path){
    if(path.begin()->string() != "/"){
        return mRoot->GetFile(path);
    } else {
        std::filesystem::path subPath;
        for(auto it = (++path.begin()); it != path.end(); it++) subPath  = subPath / it->string();
        return mRoot->GetFile(subPath);
    }
}

std::shared_ptr<Folder> FileSystem::ParseDirectory(bStream::CStream& strm, std::vector<std::shared_ptr<File>>& files, uint16_t id, std::string path){
	std::shared_ptr<Folder> dir = std::make_shared<Folder>();
	dir->mID = id;
	dir->mName = path;
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
			dir->mFolders.push_back(ParseDirectory(strm, files, subdirOff, name));

		} else {
			if(fileId < files.size()){
				files.at(fileId)->SetName(name);
				files.at(fileId)->SetID(fileId);
				dir->mFiles.push_back(files.at(fileId));
				//tempDir.mFiles.insert({name, fileId});
			}
			fileId++;
		}
	}

	dir->mFolders.shrink_to_fit();
	dir->mFiles.shrink_to_fit();

	return dir;
}

std::shared_ptr<Folder> FileSystem::ParseFNT(bStream::CStream& strm, uint32_t fntSize, std::vector<std::shared_ptr<File>>& files){
	bStream::CMemoryStream fntBuffer = bStream::CMemoryStream(fntSize, bStream::Endianess::Little, bStream::OpenMode::In);
	strm.readBytesTo(fntBuffer.getBuffer(), fntSize);
	return ParseDirectory(fntBuffer, files, 0, "");
}

std::vector<std::pair<uint32_t, uint32_t>> FileSystem::ParseFAT(bStream::CStream& strm, uint32_t entryCount){
	std::vector<std::pair<uint32_t, uint32_t>> files = {};
	for (size_t f = 0; f < entryCount; f++){
		files.push_back({strm.readUInt32(),strm.readUInt32()});
	}

	return files;
}

}