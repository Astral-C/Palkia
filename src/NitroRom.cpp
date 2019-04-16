#include <bstream.h>
#include <NitroRom.h>


void NitroRom::getRawIcon(PalkiaColor out[32][32]){

    std::vector<PalkiaColor> palette;
    for(int p = 0; p < 16; p++){
        palette.push_back({
            static_cast<uint8_t>((banner.iconPalette[p] & 0x001F) << 3),
            static_cast<uint8_t>((banner.iconPalette[p] & 0x03E0) >> 2),
            static_cast<uint8_t>((banner.iconPalette[p] & 0x7C00) >> 7),
            static_cast<uint8_t>(p == 0 ? 0x00 : 0xFF)
        });
    }


    int curPixel = 0;
    for(int by = 0; by < 4; by++){
        for(int bx = 0; bx < 4; bx++){
            for(int y = 0; y < 8; y++){
                for(int x = 0; x < 4; x++){

                    out[(bx*8) + x*2][y + (8*by)] = palette.at(banner.iconBitmap[curPixel] & 0x0F);
                    out[(bx*8) + x*2 + 1][y + (8*by)] = palette.at(banner.iconBitmap[curPixel] & 0xF0 >> 4);

                    curPixel++;
                }
            }
        }
    }
}

NitroRom::NitroRom(bStream::CFileStream& romFile){
    header = romFile.readStruct<NitroRomHeader>();
    
    romFile.seek(header.iconBannerOffset);
    banner = romFile.readStruct<NitroBanner>();

    //TODO: FNT Class and FAT Class
    /*
    romFile.seek(header.FNTOffset);
    NitroDirEntry root = romFile.readStruct<NitroDirEntry>();
    fnt.addDirectory(root);
    
    for(size_t i = 0; i < root.dirParentID-1; i++)
    {
        NitroDirEntry curDir = romFile.readStruct<NitroDirEntry>();
        fnt.addDirectory(curDir);
    }
    */
};

NitroRomHeader NitroRom::getHeader(){
    return header;
}

NitroBanner NitroRom::getBanner(){
    return banner;
}