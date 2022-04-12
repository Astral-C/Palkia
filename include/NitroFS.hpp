#pragma once
#include <map>
#include <string>
#include <vector>
#include <memory>
#include "../bstream/bstream.h"
#include <filesystem>

struct FSEntry {
	uint16_t id;
	std::string name;
	std::shared_ptr<uint8_t[]> data;
};

struct FSDir {
	uint16_t id;
	std::string name;
	std::map<std::string, FSDir> dirs;
	std::map<std::string, FSEntry> files;
};

class NitroFS {

	public:
		FSEntry* getFileByPath(std::filesystem::path);
	
        void parseRoot(bStream::CStream& strm, size_t fnt_offset, size_t fnt_size, size_t fat_offset, size_t fat_size);

		NitroFS();
		~NitroFS();

	private:
		FSDir parseDirectory(bStream::CStream&, std::vector<std::shared_ptr<uint8_t[]>> fat, uint16_t, std::string);
		FSDir root;

};