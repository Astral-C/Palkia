#pragma once
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <bstream/bstream.h>
#include "System/FileSystem.hpp"
#include <filesystem>
#include "Util.hpp"

namespace Palkia::Nitro {

#pragma pack(push,1)
typedef struct RomHeader{
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
} RomHeader;

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
} Banner;
#pragma pack(pop)


class Rom {
	private:
		RomHeader mHeader;
		Banner mBanner;
		FileSystem mFS;
	public:
		RomHeader GetHeader();
		Banner GetBanner();

		std::shared_ptr<File> GetFile(std::filesystem::path);

		void Dump();

		Rom(std::filesystem::path);
		void GetRawIcon(Color out[32][32]);

		~Rom();
};

}