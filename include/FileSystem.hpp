#pragma once
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <bstream/bstream.h>
#include <filesystem>

namespace Palkia::Nitro {

struct File {
	uint32_t size;
	uint8_t* data;
};

struct Directory {
	uint16_t id;
	std::string name;
	std::map<std::string, Directory> mDirectories;
	std::map<std::string, uint16_t> mFiles;
};

class FileSystem {

	public:
		File* GetFileByPath(std::filesystem::path);
		File* GetFileByIndex(size_t);
        void ParseRoot(bStream::CStream& strm, size_t fnt_offset, size_t fnt_size, size_t fat_offset, size_t fat_size, size_t img_offset, bool has_fnt = true);

		Directory* GetRoot();

		FileSystem();
		~FileSystem();

	private:
		std::vector<File> mFiles; // probably shouldnt be like this... 
		Directory mRoot;
		Directory ParseDirectory(bStream::CStream&, uint16_t, std::string);

};

}