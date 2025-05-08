#include <algorithm>
#include "NDS/System/Rom.hpp"
#include "Util.hpp"
#include <format>
#include <cstddef>

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
		bStream::CFileStream romFile(p, bStream::Endianess::Little, bStream::OpenMode::In);
		mHeader = romFile.readStruct<RomHeader>();
		romFile.seek(mHeader.iconBannerOffset, false);
		mBanner = romFile.readStruct<Banner>();

		std::cout << "Reading FAT at " << std::hex << mHeader.FATOffset << std::dec << std::endl;
		romFile.seek(mHeader.FATOffset);
		std::vector<std::shared_ptr<File>> files;
		uint32_t id = 0;
		for(auto file : mFS.ParseFAT(romFile, (mHeader.FATSize / 8))){
			files.push_back(File::Load(romFile, id++, file.first, file.second));
		}

		romFile.seek(mHeader.FNTOffset);
		mFS.mRoot = mFS.ParseFNT(romFile, mHeader.FNTSize, files);

		// fucking whatever.
		//std::sort(files.begin(), files.end(), [](std::shared_ptr<File> a, std::shared_ptr<File> b){ return a->GetID() < b->GetID(); });

		romFile.seek(mHeader.arm9RomOff);
		uint8_t* arm9Data = new uint8_t[mHeader.arm9Size];
		romFile.readBytesTo(arm9Data, mHeader.arm9Size);

		if(romFile.peekUInt32(romFile.tell()) == 0xDEC00621){
			mNitroFooter[0] = romFile.readUInt32();
			mNitroFooter[1] = romFile.readUInt32();
			mNitroFooter[2] = romFile.readUInt32();
		}

		romFile.seek(mHeader.arm7RomOff);
		uint8_t* arm7Data = new uint8_t[mHeader.arm7Size];
		romFile.readBytesTo(arm7Data, mHeader.arm7Size);

		romFile.seek(mHeader.debugRomOffset);
		uint8_t* debugRomData = new uint8_t[mHeader.debugRomSize];
		romFile.readBytesTo(debugRomData, mHeader.debugRomSize);

		mRomFiles = std::make_shared<Folder>();

		auto overlay9Dir = std::make_shared<Folder>();
		overlay9Dir->SetName("overlays9");
		mRomFiles->AddFolder(overlay9Dir);

		auto overlay7Dir = std::make_shared<Folder>();
		overlay7Dir->SetName("overlays7");
		mRomFiles->AddFolder(overlay7Dir);
		
		auto arm9 = File::Create();
		arm9->SetName("arm9.bin");
		arm9->SetData(arm9Data, mHeader.arm9Size);

		auto arm7 = File::Create();
		arm7->SetName("arm7.bin");
		arm7->SetData(arm7Data, mHeader.arm7Size);

		if(debugRomData != 0){
			auto debugRom = File::Create();
			debugRom->SetName("debug.nds");
			debugRom->SetData(debugRomData, mHeader.debugRomSize);
			mRomFiles->AddFile(debugRom);
		}

		mRomFiles->AddFile(arm9);
		mRomFiles->AddFile(arm7);

		if(mHeader.arm9OverlayOffset != 0){
			romFile.seek(mHeader.arm9OverlayOffset);
			romFile.seek(mHeader.arm9OverlayOffset);
			std::cout << "Reading ARM9 Overlays at " << std::hex << mHeader.arm9OverlayOffset << " ??? " << romFile.tell() << std::dec << std::endl; 
			mOverlays9.resize(mHeader.arm9OverlaySize / 32);
			for(std::size_t i = 0; i < mHeader.arm9OverlaySize; i += 32){
				uint32_t overlayID = romFile.readUInt32();
				mOverlays9[overlayID].overlayID = overlayID;
				mOverlays9[overlayID].ramAddress = romFile.readUInt32();
				mOverlays9[overlayID].ramSize = romFile.readUInt32();
				mOverlays9[overlayID].bssSize = romFile.readUInt32();
				mOverlays9[overlayID].staticInitStart = romFile.readUInt32();
				mOverlays9[overlayID].staticInitEnd = romFile.readUInt32();
				mOverlays9[overlayID].fileID = romFile.readUInt32();
				uint32_t compressedFlags = romFile.readUInt32();
				mOverlays9[overlayID].compressedSize = compressedFlags & 0xFFFFFF;
				mOverlays9[overlayID].flags = compressedFlags >> 24;
				

				mOverlays9[overlayID].file = overlay9Dir->AddFile(files[mOverlays9[overlayID].fileID]);
			}
		}

		if(mHeader.arm7OverlayOffset != 0){
			uint32_t maxOverlayFile = 0;
			romFile.seek(mHeader.arm7OverlayOffset);
			mOverlays7.resize(mHeader.arm7OverlaySize / 32);
			for(std::size_t i = 0; i < mHeader.arm7OverlaySize; i += 32){
				uint32_t overlayID = romFile.readUInt32();
				mOverlays7[overlayID].overlayID = overlayID;
				mOverlays7[overlayID].ramAddress = romFile.readUInt32();
				mOverlays7[overlayID].ramSize = romFile.readUInt32();
				mOverlays7[overlayID].bssSize = romFile.readUInt32();
				mOverlays7[overlayID].staticInitStart = romFile.readUInt32();
				mOverlays7[overlayID].staticInitEnd = romFile.readUInt32();
				mOverlays7[overlayID].fileID = romFile.readUInt32();
				uint32_t compressedFlags = romFile.readUInt32();
				mOverlays7[overlayID].compressedSize = compressedFlags & 0xFFFFFF;
				mOverlays7[overlayID].flags = compressedFlags >> 24;

				mOverlays7[overlayID].file = overlay7Dir->AddFile(files[mOverlays7[overlayID].fileID]);
			}
		}

		uint32_t rsaSigOffset = 0;
		if(romFile.getSize() > 0x1004){
			uint32_t rsaSigOffset = romFile.peekInt32(0x1000);
			if(!rsaSigOffset && romFile.getSize() > mHeader.totalUsedRom){
				rsaSigOffset = mHeader.totalUsedRom;
			}
		}
		
		if(rsaSigOffset != 0){
			romFile.seek(rsaSigOffset);
			romFile.readBytesTo(mRsaSig.data(), rsaSigOffset + 0x88 > romFile.getSize() ? romFile.getSize() - rsaSigOffset : rsaSigOffset + 0x88);
			mHasSig = true;
		}

		delete[] arm9Data;
		delete[] arm7Data;
		delete[] debugRomData;
	} else {
		std::printf("File %s not found.\n", p.filename().c_str());
	}
}

void Rom::Save(std::filesystem::path p){
	bStream::CFileStream romFile(p, bStream::Endianess::Little, bStream::OpenMode::Out);

	romFile.seek(0x4000);
	auto arm9 = mRomFiles->GetFile("arm9.bin");
	if(arm9){
		mHeader.arm9RomOff = romFile.tell();
		mHeader.arm9Size = arm9->GetSize();
		romFile.writeBytes(arm9->GetData(), arm9->GetSize());
	} else {
		mHeader.arm9RomOff = 0;
		mHeader.arm9Size = 0;
	}

	if(mNitroFooter[0] == 0xDEC00621){
		romFile.writeUInt32(mNitroFooter[0]);
		romFile.writeUInt32(mNitroFooter[1]);
		romFile.writeUInt32(mNitroFooter[2]);
	}

	if(romFile.tell() < 0x8000){
		romFile.seek(0x8000);
	}


	auto arm7 = mRomFiles->GetFile("arm7.bin");
	if(arm7){
		mHeader.arm7RomOff = romFile.tell();
		mHeader.arm7Size = arm7->GetSize();
		romFile.writeBytes(arm7->GetData(), arm7->GetSize());
	} else {
		mHeader.arm7RomOff = 0;
		mHeader.arm7Size = 0;
	}
	
	while((romFile.tell() % 0x400) != 0) romFile.writeUInt8(0xFF);

	mHeader.iconBannerOffset = romFile.tell();
	romFile.writeBytes((uint8_t*)&mBanner, sizeof(mBanner)); // this can change for dsi but idc rn oops

	// Messy but it works
	uint32_t fntSize = mFS.CalculateFNTSize();

	uint8_t* fntData = new uint8_t[fntSize](0xFF);
	bStream::CMemoryStream fntStream(fntData, fntSize, bStream::Endianess::Little, bStream::OpenMode::Out);
	mFS.WriteFNT(fntStream); // this resets all file ids, this should probably be its own function - regenerate IDs or something - that gets called whenever a modification happens to the filesystem

	while((romFile.tell() % 0x400) != 0) romFile.writeUInt8(0xFF);
	mHeader.FNTOffset = romFile.tell();
	mHeader.FNTSize = fntSize;
	romFile.writeBytes(fntData, fntSize);
	
	delete[] fntData;
	
	while((romFile.tell() % 0x400) != 0) romFile.writeUInt8(0xFF);
	mHeader.FATOffset = romFile.tell();
	mHeader.FATSize = mFS.CalculateFATSize() + (mOverlays9.size() * 8) + (mOverlays7.size() * 8);
	romFile.seek(romFile.tell() + mHeader.FATSize);

	std::vector<std::pair<uint32_t, uint32_t>> tempFAT = {};
	
	mFS.ForEachFile([&](std::shared_ptr<Palkia::Nitro::File> file){
		std::pair<uint32_t, uint32_t> fatEntry {romFile.tell(), 0};
		romFile.writeBytes(file->GetData(), file->GetSize());
		fatEntry.second = romFile.tell();

		tempFAT.push_back(fatEntry);
	});

	for (std::size_t i = 0; i < mOverlays9.size(); i++){
		// Setup the overlay
		if(mOverlays9[i].file.lock()){
			auto file = mOverlays9[i].file.lock();

			mOverlays9[i].fileID = tempFAT.size();

			//write the file and add it to the FAT
			std::pair<uint32_t, uint32_t> fatEntry {romFile.tell(), 0};
			romFile.writeBytes(file->GetData(), file->GetSize());
			fatEntry.second = romFile.tell();

			tempFAT.push_back(fatEntry);
		}
	}

	for (std::size_t i = 0; i < mOverlays7.size(); i++){
		// Setup the overlay
		if(mOverlays7[i].file.lock()){
			auto file = mOverlays7[i].file.lock();

			mOverlays7[i].fileID = tempFAT.size();

			//write the file and add it to the FAT
			std::pair<uint32_t, uint32_t> fatEntry {romFile.tell(), 0};
			romFile.writeBytes(file->GetData(), file->GetSize());
			fatEntry.second = romFile.tell();

			tempFAT.push_back(fatEntry);
		}
	}

	while((romFile.tell() % 0x400) != 0) romFile.writeUInt8(0xFF);

	// Save our spot for writing the overlay data, we need to go back and write the FAT
	if(mOverlays9.size() > 0){
		mHeader.arm9OverlayOffset = romFile.tell();
		mHeader.arm9OverlaySize = mOverlays9.size() * 0x20;
	} else {
		mHeader.arm9OverlayOffset = 0;
		mHeader.arm9OverlaySize = 0;
	}

	romFile.seek(mHeader.FATOffset);
	for (auto [start, end] : tempFAT){
		romFile.writeUInt32(start);
		romFile.writeUInt32(end);
	}

	romFile.seek(mHeader.arm9OverlayOffset);

	for (std::size_t i = 0; i < mOverlays9.size(); i++){
		romFile.writeUInt32(mOverlays9[i].overlayID);
		romFile.writeUInt32(mOverlays9[i].ramAddress);
		romFile.writeUInt32(mOverlays9[i].ramSize);
		romFile.writeUInt32(mOverlays9[i].bssSize);
		romFile.writeUInt32(mOverlays9[i].staticInitStart);
		romFile.writeUInt32(mOverlays9[i].staticInitEnd);
		romFile.writeUInt32(mOverlays9[i].fileID);
		romFile.writeUInt32(mOverlays9[i].compressedSize | ((mOverlays9[i].flags << 24) & 0xFF)); //bad
	}

	while((romFile.tell() % 0x400) != 0) romFile.writeUInt8(0xFF);

	if(mOverlays7.size() > 0){
		mHeader.arm7OverlayOffset = romFile.tell();
		mHeader.arm7OverlaySize = mOverlays7.size() * 0x20;
	} else {
		mHeader.arm7OverlayOffset = 0;
		mHeader.arm7OverlaySize = 0;
	}	


	for (std::size_t i = 0; i < mOverlays7.size(); i++){
		romFile.writeUInt32(mOverlays7[i].overlayID);
		romFile.writeUInt32(mOverlays7[i].ramAddress);
		romFile.writeUInt32(mOverlays7[i].ramSize);
		romFile.writeUInt32(mOverlays7[i].bssSize);
		romFile.writeUInt32(mOverlays7[i].staticInitStart);
		romFile.writeUInt32(mOverlays7[i].staticInitEnd);
		romFile.writeUInt32(mOverlays7[i].fileID);
		romFile.writeUInt32(mOverlays7[i].compressedSize | ((mOverlays9[i].flags << 24) & 0xFF)); //bad
	}

	while((romFile.tell() % 0x400) != 0) romFile.writeUInt8(0xFF);

	auto debugRom = mRomFiles->GetFile("debug.nds");
	if(debugRom != nullptr){
		mHeader.debugRomOffset = romFile.tell();
		mHeader.debugRomSize = debugRom->GetSize();
		romFile.writeBytes(debugRom->GetData(), debugRom->GetSize());
	} else {
		mHeader.debugRomOffset = 0;
		mHeader.debugRomSize = 0;
	}

	while((romFile.tell() % 0x400) != 0) romFile.writeUInt8(0xFF);
	
	mHeader.totalUsedRom = romFile.tell();
	
	mHeader.devCapacity = 0;
	
	while(((uint64_t)128000 << (uint64_t)mHeader.devCapacity) < mHeader.totalUsedRom) { mHeader.devCapacity += 1; }

	// TODO: checksum
	//mHeader.headerChecksum = 0xFFFF;

	romFile.seek(0x00);
	romFile.writeBytes((uint8_t*)&mHeader, sizeof(mHeader));
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
	if(path.string().at(0) == '@'){
		return mRomFiles->GetFile(std::filesystem::path(path.string().substr(1,path.string().size())));
	}
	return mFS.GetFile(path);
}

}
