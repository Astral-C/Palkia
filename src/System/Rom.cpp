#include <algorithm>
#define BSTREAM_IMPLEMENTATION
#include "System/Rom.hpp"
#include "Util.hpp"

namespace Palkia::Nitro {

void Rom::GetRawIcon(Color out[32][32]){

	std::vector<Color> palette;
	for(int p = 0; p < 16; p++){
		palette.push_back({
			static_cast<uint8_t>((mBanner.iconPalette[p] & 0x001F) << 3),
			static_cast<uint8_t>((mBanner.iconPalette[p] & 0x03E0) >> 2),
			static_cast<uint8_t>((mBanner.iconPalette[p] & 0x7C00) >> 7),
			static_cast<uint8_t>(p == 0 ? 0x00 : 0xFF)
		});
	}

	uint8_t iconBitmapExpanded[0x200 * 2];
	for(int b = 0; b < 0x200; b ++){
		iconBitmapExpanded[b * 2] = mBanner.iconBitmap[b] & 0x0F;
		iconBitmapExpanded[b * 2 + 1] = (mBanner.iconBitmap[b] & 0xF0) >> 4;
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

Rom::Rom(std::filesystem::path p){
	if(std::filesystem::exists(p)){
		bStream::CFileStream romFile(p.relative_path(), bStream::Endianess::Little, bStream::OpenMode::In);
		mHeader = romFile.readStruct<RomHeader>();
		romFile.seek(mHeader.iconBannerOffset, false);
		mBanner = romFile.readStruct<Banner>();

		romFile.seek(mHeader.FATOffset);
		std::vector<std::shared_ptr<File>> files;
		int id = 0;
		for(auto file : mFS.ParseFAT(romFile, (mHeader.FATSize / 8) - 1)){
			files.push_back(File::Load(romFile, id++, file.first, file.second));
		}


		romFile.seek(mHeader.FNTOffset);
		mFS.mRoot = mFS.ParseFNT(romFile, mHeader.FNTSize, files);

	} else {
		std::printf("File %s not found.\n", p.filename().c_str());
	}
}

Rom::~Rom(){}

void Rom::Dump(){
	std::string name = std::string(mHeader.romID);
	std::replace(name.begin(), name.end(), ' ', '_');
    std::filesystem::create_directory(name);
	mFS.GetRoot()->Dump("");
}


RomHeader Rom::GetHeader(){
	return mHeader;
}

Banner Rom::GetBanner(){
	return mBanner;
}

std::shared_ptr<File> Rom::GetFile(std::filesystem::path path){
	return mFS.GetFile(path);
}

}
