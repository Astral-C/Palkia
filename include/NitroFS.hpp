#pragma once
#include <map>
#include <string>
#include <vector>
#include <memory>
#include "../bstream/bstream.h"
#include <filesystem>

struct NitroFile {
	uint32_t size;
	uint8_t* data;
};

struct FSDir {
	uint16_t id;
	std::string name;
	std::map<std::string, FSDir> dirs;
	std::map<std::string, uint16_t> files;
};

class NitroFS {

	public:
		NitroFile* getFileByPath(std::filesystem::path);
		NitroFile* NitroFS::getFileByIndex(size_t);
        void parseRoot(bStream::CStream& strm, size_t fnt_offset, size_t fnt_size, size_t fat_offset, size_t fat_size, size_t img_offset, bool has_fnt = true);

		FSDir* getRoot();

		NitroFS();
		~NitroFS();

	private:
		std::vector<NitroFile> files; 
		FSDir parseDirectory(bStream::CStream&, uint16_t, std::string);
		FSDir root;

};