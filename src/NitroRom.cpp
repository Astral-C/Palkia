#include "NitroRom.hpp"
#include "util.hpp"

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

		fs.parseRoot(romFile, header.FNTOffset, header.FNTSize, header.FATOffset, header.FATSize, 0);

	} else {
		std::printf("File %s not found.\n", p.filename().c_str());
	}
}

NitroRom::~NitroRom(){}

NitroRomHeader NitroRom::getHeader(){
	return header;
}

NitroBanner NitroRom::getBanner(){
	return banner;
}

NitroFile* NitroRom::getFileByPath(std::filesystem::path path){
	return fs.getFileByPath(path);
}

}

