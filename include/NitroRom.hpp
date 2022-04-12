#pragma once
#include <map>
#include <string>
#include <vector>
#include <memory>
#include "../bstream/bstream.h"
#include "NitroFS.hpp"
#include <filesystem>
#include "util.hpp"

namespace Palkia {

#pragma pack(push,1)
typedef struct NitroRomHeader{
	char romID[12];
	char gameCode[4];
	char makerCode[2];
	uint8_t unitCode;
	uint8_t encryptionSeedSelect;
	uint8_t devCapacity;
	char reserved[7];
	uint8_t reservedDsi;
	uint8_t region;
	uint8_t version;
	uint8_t flags;
	uint32_t arm9RomOff;
	uint32_t arm9EntryAddr;
	uint32_t arm9loadAddr;
	uint32_t arm9Size;
	uint32_t arm7RomOff;
	uint32_t arm7EntryAddr;
	uint32_t arm7loadAddr;
	uint32_t arm7Size;

	uint32_t FNTOffset;
	uint32_t FNTSize;
	uint32_t FATOffset;
	uint32_t FATSize;

	uint32_t arm9OverlayOffset;
	uint32_t arm9OverlaySize;
	uint32_t arm7OverlayOffset;
	uint32_t arm7OverlaySize;

	uint32_t normalCCRegSettings;
	uint32_t secureCCRegSettings;

	uint32_t iconBannerOffset;
	uint16_t secureAreaCRC;
	uint16_t secureTransferTimeout;

	uint32_t arm9AutoLoad;
	uint32_t arm7AutoLoad;

	uint64_t secureDisable;
	//TODO: finish this
} NitroRomHeader;

typedef struct NitroBanner {
	uint16_t version;
	uint16_t crc16;
	char reserved[0x1C];
	uint8_t iconBitmap[0x200];
	uint16_t iconPalette[0x10];

	char japaneseTitle[0x100];
	char englishTitle[0x100];
	char frenchTitle[0x100];
	char germanTitle[0x100];
	char italianTitle[0x100];
	char spanishTitle[0x100];
} NitroBanner;
#pragma pack(pop)


class NitroRom {
	public:
		NitroRomHeader getHeader();
		NitroBanner getBanner();

		FSEntry* getFileByPath(std::filesystem::path);

		NitroRom(std::filesystem::path);
		void getRawIcon(Color out[32][32]);

		~NitroRom();

	private:
		void parseFS(NitroRomHeader&, bStream::CStream&);

		NitroRomHeader header;
		NitroBanner banner;
		NitroFS fs;

};

}