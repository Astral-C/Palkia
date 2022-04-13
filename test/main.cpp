#include <NitroRom.hpp>
#include <NitroArchive.hpp>
#include <filesystem>
#include <functional>
#include "tests.h"

int main(){
	Tests tests;

	
	/*tests.registerTest("Get file from rom", [](){

		Palkia::NitroRom Platinum(std::filesystem::path("platinum.nds"));
		NitroFile* zone_event_narc = Platinum.getFileByPath("fielddata/eventdata/zone_event.narc");
		

		if(zone_event_narc == nullptr){
			std::cout << "Didn't find file :(" << std::endl;
			return false;
		} else {
			return true;
		}
	});*/

	tests.registerTest("Get file from narc", [](){

		bStream::CFileStream narc_file("arc0.narc", bStream::Endianess::Little, bStream::OpenMode::In);
		NitroArchive archive(narc_file);

		archive.dump();

		return true;
	});

	tests.runTests();
}
