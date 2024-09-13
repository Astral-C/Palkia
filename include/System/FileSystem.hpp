#pragma once
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <bstream/bstream.h>
#include <filesystem>

namespace Palkia::Nitro {

class Rom;
class FileSystem;
class Archive;

class File : public std::enable_shared_from_this<File> {
	uint16_t mID;
	std::string mName;
	uint32_t mSize { 0 };
	uint8_t* mData { nullptr };

public:

	uint32_t GetSize() { return mSize; }
	uint8_t* GetData(){ return mData; }

	void SetID(uint16_t id) { mID = id; }
	void SetName(std::string name) { mName = name; }

	std::string GetName() { return mName; }

	static std::shared_ptr<File> Create() { return std::make_shared<File>(); }

    static std::shared_ptr<File> Load(bStream::CStream& strm, uint16_t id, uint32_t start, uint32_t end){
        std::shared_ptr<File> f = std::make_shared<File>();

		f->mName = std::format("{}.bin", id);
		f->mData = new uint8_t[end - start];
		f->mSize = end - start;

		strm.seek(start);
		strm.readBytesTo(f->mData, f->mSize);

		return f;
    }
        
    std::shared_ptr<File> GetPtr(){
        return shared_from_this();
    }

	File() {}
	~File() {
		if(mData != nullptr){
			delete[] mData;
		}
	}
};

class Folder : public std::enable_shared_from_this<Folder> {
	friend FileSystem;
	
	std::weak_ptr<Folder> mParent;
	std::weak_ptr<FileSystem> mFileSystem;

	uint16_t mID;
	std::string mName;
	std::vector<std::shared_ptr<File>> mFiles;
	std::vector<std::shared_ptr<Folder>> mFolders;

public:

	void SetParent(std::shared_ptr<Folder> parent) { mParent = parent; }


	std::string GetName() { return mName; }
	std::shared_ptr<File> GetFile(std::filesystem::path);

	void Dump(std::filesystem::path out_path);

	static std::shared_ptr<Folder> Create(std::shared_ptr<FileSystem> fs) { return std::make_shared<Folder>(fs); }

    std::shared_ptr<Folder> GetPtr(){
        return shared_from_this();
    }

	Folder(std::shared_ptr<FileSystem> fs){
		mFileSystem = fs;
	}
	Folder(){} // this should be removed later
	~Folder(){}
};

class FileSystem {
	friend Rom;
	friend Archive;
private:
	bool mHasFNT { true };
	uint32_t mNextFileID { 0 };
	std::shared_ptr<Folder> mRoot;
	std::vector<std::shared_ptr<File>> mFiles; // only used when no FNT
	std::shared_ptr<Folder> ParseDirectory(bStream::CStream& strm, std::vector<std::shared_ptr<File>>& files, uint16_t id, std::string path);
public:
	std::shared_ptr<Folder> GetRoot(){ return mRoot; }
	std::shared_ptr<File> GetFile(std::filesystem::path);
	
	std::vector<std::pair<uint32_t, uint32_t>> ParseFAT(bStream::CStream& strm, uint32_t entryCount);
	std::shared_ptr<Folder> ParseFNT(bStream::CStream& strm, uint32_t fntSize, std::vector<std::shared_ptr<File>>& files);

	FileSystem();
	~FileSystem();
};

}