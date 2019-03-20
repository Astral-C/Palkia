#include <bstream.h>
#include <NitroRom.h>
#include <NitroFS.h>

NitroRom::NitroRom(std::filesystem::path p){
    if(std::filesystem::exists(p)){
        bStream::CFileStream romFile(p.relative_path());
        header = romFile.readStruct<NitroRomHeader>();
        
        //TODO: FNT Class and FAT Class
        romFile.seek(header.FNTOffset);
        NitroDirEntry root = ReadDirEntry(&romFile);
        
        std::printf("Total Folder Count %d\n", root.dirParentID);
        
        for(size_t i = 0; i < root.dirParentID-1; i++)
        {
            NitroDirEntry curDir = ReadDirEntry(&romFile);
        }
        
        
    } else {
        std::printf("File %s not found.\n", p.filename().c_str());
    }
};

NitroRomHeader NitroRom::getHeader(){
    return header;
}