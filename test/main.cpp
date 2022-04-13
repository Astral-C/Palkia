#include <NitroRom.hpp>
#include <NitroArchive.hpp>
#include <filesystem>
#include <functional>
#include "tests.h"

int main(){
	Tests tests;

	
	tests.registerTest("Get file from rom", [](){

		Palkia::NitroRom Platinum(std::filesystem::path("platinum.nds"));
		NitroFile* zone_event_narc = Platinum.getFileByPath("fielddata/eventdata/zone_event.narc");
		

		if(zone_event_narc == nullptr){
			std::cout << "Didn't find file :(" << std::endl;
			return false;
		} else {
			bStream::CFileStream out = bStream::CFileStream("out.bin", bStream::OpenMode::Out);
			bStream::CMemoryStream stream = bStream::CMemoryStream(zone_event_narc->data, zone_event_narc->size, bStream::Little, bStream::OpenMode::In);
			NitroArchive archive(stream);
			archive.getFileByIndex(0);
			return true;
		}
	});

	tests.runTests();
}
