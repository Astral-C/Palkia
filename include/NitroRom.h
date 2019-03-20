#include <string>
#include <filesystem>

typedef struct NitroRomHeader{
    char romID[12];
    char gameCode[4];
    char makerCode[2];
    uint8_t unitCode;
    uint8_t encryptionSeedSelect;
    uint8_t devCapacity;
    char reserved[7];
    uint16_t revision;
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

class NitroRom {
    public:
        NitroRomHeader getHeader();
        NitroRom(std::filesystem::path);
        ~NitroRom(){}

    private:
        NitroRomHeader header;

};