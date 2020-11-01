#include <NitroRom.h>
#include <filesystem>
#include <functional>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "tests.h"

int main(){
	Tests tests;

	tests.registerTest("Load Rom, Dump Icon", [](){

		Palkia::NitroRom Platinum(std::filesystem::path("test/files/platinum.nds"));
		std::printf("Game Code is %s\n", Platinum.getHeader().gameCode);

		Palkia::Color iconBitmap[32][32];
		Platinum.getRawIcon(iconBitmap);

		stbi_write_png("icon.png", 32, 32, 4, (void*)iconBitmap, 4*32);

		return true;
	});

	
	tests.registerTest("Get file from rom", [](){

		Palkia::NitroRom Platinum(std::filesystem::path("test/files/platinum.nds"));
		std::shared_ptr<bStream::CMemoryStream> field_data = Platinum.getFileByPath("fielddata/land_data/land_data.narc");
		if(field_data == nullptr){
			std::cout << "Didn't find file :(" << std::endl;
			return false;
		} else {
			std::cout << "Should be NARC: ";
			std::cout << field_data->readString(4) << std::endl;
			return true;
		}
	});

	tests.runTests();
}
