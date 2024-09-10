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
			
			bStream::CMemoryStream zone_event_stream(zone_event_narc->data, zone_event_narc->size, bStream::Endianess::Little, bStream::OpenMode::In);
			NitroArchive archive(zone_event_stream, true);
			std::cout << ((uint32_t*)archive.getFileByIndex(0)->data)[0] << std::endl;
			return true;
		}
	});

	tests.runTests();
}
