#pragma once
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <bstream/bstream.h>
#include "NDS/System/FileSystem.hpp"
#include <filesystem>
#include "Util.hpp"

namespace Palkia::Nitro {

#pragma pack(push,1)
typedef struct RomHeader {
	char romID[12];
	uint32_t gameCode;
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
	uint32_t totalUsedRom;
	uint32_t romHeaderSize;
	uint8_t reserved2[0x38];
	uint8_t nintendoLogoData[0x9C];
	uint16_t nintendoLogoChecksum;
	uint16_t headerChecksum;
	uint32_t debugRomOffset;
	uint32_t debugRomSize;
	uint32_t debugRamAddress;
	uint32_t reserved3;
	uint8_t reserved4[0x90];
} RomHeader;

typedef struct NitroBanner {
	uint16_t version;
	uint16_t crc16_part1;
	uint16_t crc16_part2;
	uint16_t crc16_part3;
	uint16_t crc16_part4;
	char reserved[0x16];
	uint8_t iconBitmap[0x200];
	uint16_t iconPalette[0x10];

	uint8_t japaneseTitle[0x100];
	uint8_t englishTitle[0x100];
	uint8_t frenchTitle[0x100];
	uint8_t germanTitle[0x100];
	uint8_t italianTitle[0x100];
	uint8_t spanishTitle[0x100];
} Banner;
#pragma pack(pop)

struct Overlay {
	uint32_t overlayID;
	uint32_t ramAddress;
	uint32_t ramSize;
	uint32_t bssSize;
	uint32_t staticInitStart;
	uint32_t staticInitEnd;
	uint32_t fileID;
	uint32_t compressedSize;
	uint32_t flags;
	std::weak_ptr<File> file;
};

class Rom {
	private:
		RomHeader mHeader;
		Banner mBanner;
		FileSystem mFS;

		std::vector<Overlay> mOverlays7;
		std::vector<Overlay> mOverlays9;

		bool mHasSig { false };
		bool mArm9Compressed { false };
		std::array<uint8_t, 0x88> mRsaSig;
		std::array<uint32_t, 3> mNitroFooter; // no idea what this is supposed to be

		// this contains things like arm9 as  files
		std::shared_ptr<Folder> mRomFiles = nullptr;

	public:
		RomHeader GetHeader();
		Banner GetBanner();

		std::shared_ptr<File> GetFile(std::filesystem::path);

		FileSystem GetFS() { return mFS; }

		void Dump();

		Rom(std::filesystem::path);
		void Save(std::filesystem::path);
		void GetRawIcon(Color out[32][32]);

		~Rom();
};

}