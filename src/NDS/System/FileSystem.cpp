#include "Util.hpp"
#include "NDS/System/FileSystem.hpp"

namespace Palkia::Nitro {

void File::SetData(uint8_t* data, size_t size){
	if(mData != nullptr){
		delete[] mData;
	}

	mSize = size;
	mData = new uint8_t[size];
	memcpy(mData, data, size);
}

void Folder::AddFile(std::shared_ptr<File> file){
	mFiles.push_back(file);
}

void Folder::Dump(std::filesystem::path out_path){
	for (auto& directory : mFolders){
        std::filesystem::create_directory(out_path / directory->GetName());
        directory->Dump(out_path / directory->GetName());
    }

    for (auto& file : mFiles){
        bStream::CFileStream out(out_path / file->GetName(), bStream::OpenMode::Out);
        out.writeBytes(file->GetData(), file->GetSize());
    }
    
}

void Folder::ForEachFile(std::function<void(std::shared_ptr<File>)> OnFile){
	for (auto folder : mFolders){
		folder->ForEachFile(OnFile);
	}
		
	for (auto file : mFiles){
		OnFile(file);
	}
}

void Folder::Traverse(std::function<void(std::shared_ptr<Folder>)> OnFolder, std::function<void(std::shared_ptr<File>)> OnFile){
	OnFolder(GetPtr());
	for (auto folder : mFolders){
		folder->ForEachFile(OnFile);
	}
		
	for (auto file : mFiles){
		OnFile(file);
	}
}

FileSystem::FileSystem(){}
FileSystem::~FileSystem(){}

void FileSystem::Traverse(std::function<void(std::shared_ptr<Folder>)> OnFolder, std::function<void(std::shared_ptr<File>)> OnFile){
	if(!mHasFNT){
		ForEachFile(OnFile);
	} else {
		mRoot->Traverse(OnFolder, OnFile);
	}
}

void FileSystem::ForEachFile(std::function<void(std::shared_ptr<File>)> OnFile){
	if(!mHasFNT){
		for (auto file : mFiles){
			OnFile(file);
		}
	}
	else {
		mRoot->ForEachFile(OnFile);
	}
	

}


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

	if(dir->mFiles.size() == 0 && dir->mFolders.size() == 0){
		return nullptr; // FNT is empty!
	}

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

void FileSystem::WriteDirectory(bStream::CStream& strm, std::shared_ptr<Folder> dir){
	strm.writeUInt32(strm.getSize() & 0x0FFF); // offset to dir data
	if(dir->mFiles.size() > 0){
		strm.writeUInt16(dir->mFiles[0]->GetID());
	} else {
		strm.writeUInt16(0); // not sure this is right behavior
	}

	uint32_t pos = strm.tell();
	strm.seek(strm.getSize());
	for (uint32_t d = 0; d < dir->mFolders.size(); d++){
		uint8_t nameLen = (dir->mFolders[d]->mName.size() & 0x7F) | 0x80;
		strm.writeString(dir->mFolders[d]->mName);
		strm.writeUInt16(dir->mFolders[d]->mID); // offset
	}

	for (uint32_t f = 0; f < dir->mFiles.size(); f++){
		uint8_t nameLen = 0x00 | (dir->mFiles[f]->GetName().size() & 0x7F);
		strm.writeString(dir->mFiles[f]->GetName());
		strm.writeUInt16(dir->mFiles[f]->GetID());
	}
	strm.seek(pos);
}


uint32_t FileSystem::CalculateFNTSize(){
	uint32_t fntSize = 0;

	Traverse(
		[&](std::shared_ptr<Folder> f){
			fntSize += 9 + f->GetName().size();
		},
		[&](std::shared_ptr<File> f){
			fntSize += 9 + f->GetName().size();
		}
	);

	return fntSize;
}

void FileSystem::WriteFNT(bStream::CStream& strm){
	if(!mHasFNT){
		// write dummy FNT data
		return;
	}

	// Calculate FNT Size

	// Write the FNT

	WriteDirectory(strm, mRoot);

}

uint32_t FileSystem::CalculateFATSize(){
	uint32_t fatSize = 0;

	ForEachFile([&](std::shared_ptr<File> f){
		fatSize += 8;
	});

	return fatSize;
}

void FileSystem::WriteFAT(bStream::CStream& strm){

}


}