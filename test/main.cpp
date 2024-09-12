#include <System/Rom.hpp>
#include <System/Archive.hpp>
#include <filesystem>
#include <functional>
#include "tests.h"

int main(){
	Tests tests;

	
	tests.registerTest("Get file from rom", [](){

		Palkia::Nitro::Rom Platinum(std::filesystem::path("platinum.nds"));
		//Palkia::Nitro::File* zone_event_narc = Platinum.GetFileByPath("fielddata/eventdata/zone_event.narc");
		
		bStream::CFileStream buildModelArc("build_model.narc");

		Palkia::Nitro::Archive arc(buildModelArc);

		arc.Dump();

		//Platinum.Dump();

		/*
		if(zone_event_narc == nullptr){
			std::cout << "Didn't find file :(" << std::endl;
			return false;
		} else {
			bStream::CFileStream test("test.narc", bStream::Endianess::Little, bStream::OpenMode::Out);
			test.writeBytes(zone_event_narc->data, zone_event_narc->size);
			//bStream::CMemoryStream zone_event_stream(zone_event_narc->data, zone_event_narc->size, bStream::Endianess::Little, bStream::OpenMode::In);
			return true;
		}
		*/

		return true;
	});

	tests.runTests();
}
