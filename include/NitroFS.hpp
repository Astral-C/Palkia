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

struct FSEntry {
	uint16_t id;
	uint32_t file_id;
	std::string name;
};

struct FSDir {
	uint16_t id;
	std::string name;
	std::map<std::string, FSDir> dirs;
	std::map<std::string, FSEntry> files;
};

class NitroFS {

	public:
		NitroFile* getFileByPath(std::filesystem::path);
	
        void parseRoot(bStream::CStream& strm, size_t fnt_offset, size_t fnt_size, size_t fat_offset, size_t fat_size);

		NitroFS();
		~NitroFS();

	private:
		std::vector<NitroFile> files; 
		FSDir parseDirectory(bStream::CStream&, uint16_t, std::string);
		FSDir root;

};