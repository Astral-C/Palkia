#include <bstream.h>
#include <NitroRom.h>
#include <NitroFS.h>

PokeRom::PokeRom(std::filesystem::path p){
    if(std::filesystem::exists(p)){
        bStream::CFileStream romFile(p.relative_path());
        std::printf("Rom ID: %s\n", romFile.readString(12).c_str());
        std::printf("Game Code %s\n", romFile.readString(4).c_str());
        //TODO: FNT Class and FAT Class
        romFile.seek(0x40);
        uint32_t FNTOffset = romFile.readUInt32();
        uint32_t FNTSize = romFile.readUInt32();

        romFile.seek(FNTOffset);
        NitroDirEntry root = ReadDirEntry(&romFile);
        std::printf("Offset of Root Table: 0x%08x\nReading %d Directory Entries...\n", FNTOffset+root.dirEntryStart, root.dirParentID);
        
        romFile.seek(FNTOffset + root.dirParentID);
        for(size_t i = 0; i < root.dirParentID-1; i++)
        {
            NitroDirEntry curDir = ReadDirEntry(&romFile);
            std::printf("===Directory Entry %d===\nDirectory Offset 0x%08x\nEntry File ID 0x%04x\nParent ID: 0x%04x\n\n",
                i, curDir.dirEntryStart, curDir.dirEntryFileID, curDir.dirParentID);
        }
        


    } else {
        std::printf("File %s not found.\n", p.filename().c_str());
    }
};