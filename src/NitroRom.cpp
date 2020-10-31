#include <bstream.h>
#include <NitroRom.h>
#include "util.h"

namespace Palkia {

void NitroRom::getRawIcon(Color out[32][32]){

    std::vector<Color> palette;
    for(int p = 0; p < 16; p++){
        palette.push_back({
            static_cast<uint8_t>((banner.iconPalette[p] & 0x001F) << 3),
            static_cast<uint8_t>((banner.iconPalette[p] & 0x03E0) >> 2),
            static_cast<uint8_t>((banner.iconPalette[p] & 0x7C00) >> 7),
            static_cast<uint8_t>(p == 0 ? 0x00 : 0xFF)
        });
    }

    uint8_t iconBitmapExpanded[0x200 * 2];
    for(int b = 0; b < 0x200; b ++){
	iconBitmapExpanded[b * 2] = banner.iconBitmap[b] & 0x0F;
	iconBitmapExpanded[b * 2 + 1] = (banner.iconBitmap[b] & 0xF0) >> 4;
    }

    int pixel = 0;
    for(int by = 0; by < 4; by++){
        for(int bx = 0; bx < 4; bx++){
            for(int y = 0; y < 8; y++){
                for(int x = 0; x < 8; x++){
                    out[(by * 8) + y][(bx * 8) + x] = palette.at(iconBitmapExpanded[pixel]);
                    pixel++;
                }
            }
        }
    }
}

NitroRom::NitroRom(std::filesystem::path p){
    if(std::filesystem::exists(p)){
        bStream::CFileStream romFile(p.relative_path(), bStream::Endianess::Little, bStream::OpenMode::In);
        header = romFile.readStruct<NitroRomHeader>();
        romFile.seek(header.iconBannerOffset, false);
        banner = romFile.readStruct<NitroBanner>();

        filesystem.parseFS(header, romFile);

    } else {
        std::printf("File %s not found.\n", p.filename().c_str());
    }
};

NitroRomHeader NitroRom::getHeader(){
    return header;
}

NitroBanner NitroRom::getBanner(){
    return banner;
}

NitroFS::NitroFS(){}
NitroFS::~NitroFS(){}

FSEntry NitroFS::getFileByPath(std::filesystem::path path){
    FSDir d = root;

    for (auto &dir : path){
        if(d.dirs.count(dir) != 0){
            d = d.dirs[dir];
        }
    }

    if(d.files.count(path.filename()) != 0){
        return d.files[path.filename()];
    } else{
        return {65535, "", 65535}; //Bad
    }

}

FSDir NitroFS::parseDirectory(bStream::CStream& strm, uint16_t id, std::string path){
    FSDir tempDir;
    tempDir.id = id;
    tempDir.name = path;
    uint32_t dirOffset = strm.peekUInt32(id);
    uint16_t fileId = strm.peekUInt16(id+4);

    while(true){
        uint8_t type = strm.peekUInt8(dirOffset++);

        if(type == 0){
            break;
        }

        bool isDir = (type & 0x80);
        uint8_t nameLen = (type & 0x7F);
        std::string name = strm.peekString(dirOffset, nameLen);
        dirOffset += nameLen;

        if(isDir){
            uint16_t dirId = strm.peekUInt16(dirOffset);
            dirOffset += 2;
            uint16_t subdirOff = (dirId & 0x0FFF) * 0x08;
            tempDir.dirs.insert({name, parseDirectory(strm, subdirOff, name)});

        } else {
            tempDir.files.insert({name, {fileId, name, fileId * 0x08}});
            fileId++;
        }
    }
    return tempDir;
}

void NitroFS::parseFS(NitroRomHeader& header, bStream::CStream& strm){
    bStream::CMemoryStream fntBuffer = bStream::CMemoryStream(header.FNTSize, bStream::Endianess::Little, bStream::OpenMode::In);
    memcpy(fntBuffer.getBuffer(), strm.getBuffer() + header.FNTOffset, header.FNTSize);

    bStream::CMemoryStream fatBuffer = bStream::CMemoryStream(header.FATSize, bStream::Endianess::Little, bStream::OpenMode::In);
    memcpy(fatBuffer.getBuffer(), strm.getBuffer() + header.FATOffset, header.FATSize);

    root = parseDirectory(fntBuffer, 0, "");

    FSEntry land_data = getFileByPath("fielddata/land_data/land_data.narc");
    std::printf("%s : %d\n", land_data.name.c_str(), land_data.id);
}

}
