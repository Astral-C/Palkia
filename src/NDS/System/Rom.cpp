#include <algorithm>
#include "NDS/System/Rom.hpp"
#include "Util.hpp"
#include <format>

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
		int id = 0;
		for(auto file : mFS.ParseFAT(romFile, (mHeader.FATSize / 8) - 1)){
			files.push_back(File::Load(romFile, id++, file.first, file.second));
		}

		romFile.seek(mHeader.FNTOffset);
		mFS.mRoot = mFS.ParseFNT(romFile, mHeader.FNTSize, files);

		std::sort(files.begin(), files.end(), [](std::shared_ptr<File> a, std::shared_ptr<File> b){ return a->GetID() < b->GetID(); });

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

		auto debugRom = File::Create();
		debugRom->SetName("debug.nds");
		debugRom->SetData(debugRomData, mHeader.debugRomSize);

		mRomFiles->AddFile(arm9);
		mRomFiles->AddFile(arm7);
		mRomFiles->AddFile(debugRom);

		if(mHeader.arm9OverlayOffset != 0){
			uint32_t maxOverlayFile = 0;
			romFile.seek(mHeader.arm9OverlayOffset);
			romFile.seek(mHeader.arm9OverlayOffset);
			std::cout << "Reading ARM9 Overlays at " << std::hex << mHeader.arm9OverlayOffset << " ??? " << romFile.tell() << std::dec << std::endl; 
			mOverlays9.resize(mHeader.arm9OverlaySize / 32);
			for(int i = 0; i < mHeader.arm9OverlaySize; i += 32){
				uint32_t overlayID = romFile.readUInt32();
				mOverlays9[overlayID].overlayID = overlayID;
				mOverlays9[overlayID].ramAddress = romFile.readUInt32();
				mOverlays9[overlayID].ramSize = romFile.readUInt32();
				mOverlays9[overlayID].bssSize = romFile.readUInt32();
				mOverlays9[overlayID].staticInitStart = romFile.readUInt32();
				mOverlays9[overlayID].staticInitEnd = romFile.readUInt32();
				mOverlays9[overlayID].fileID = romFile.readUInt32();
				maxOverlayFile = mOverlays9[overlayID].fileID > maxOverlayFile ? mOverlays9[overlayID].fileID : maxOverlayFile;
				uint32_t compressedFlags = romFile.readUInt32();
				mOverlays9[overlayID].compressedSize = compressedFlags & 0xFFFFFF;
				mOverlays9[overlayID].flags = compressedFlags >> 24;
				
				mOverlays9[overlayID].file = files[mOverlays9[overlayID].fileID];

				overlay9Dir->AddFile(files[mOverlays9[overlayID].fileID]);
			}
		}

		if(mHeader.arm7OverlayOffset != 0){
			uint32_t maxOverlayFile = 0;
			romFile.seek(mHeader.arm7OverlayOffset);
			mOverlays7.resize(mHeader.arm7OverlaySize / 32);
			for(int i = 0; i < mHeader.arm7OverlaySize; i += 32){
				uint32_t overlayID = romFile.readUInt32();
				mOverlays7[overlayID].overlayID = overlayID;
				mOverlays7[overlayID].ramAddress = romFile.readUInt32();
				mOverlays7[overlayID].ramSize = romFile.readUInt32();
				mOverlays7[overlayID].bssSize = romFile.readUInt32();
				mOverlays7[overlayID].staticInitStart = romFile.readUInt32();
				mOverlays7[overlayID].staticInitEnd = romFile.readUInt32();
				mOverlays7[overlayID].fileID = romFile.readUInt32();
				maxOverlayFile = mOverlays7[overlayID].fileID > maxOverlayFile ? mOverlays7[overlayID].fileID : maxOverlayFile;
				uint32_t compressedFlags = romFile.readUInt32();
				mOverlays7[overlayID].compressedSize = compressedFlags & 0xFFFFFF;
				mOverlays7[overlayID].flags = compressedFlags >> 24;
				mOverlays7[overlayID].file = files[mOverlays7[overlayID].fileID];

				overlay7Dir->AddFile(files[mOverlays7[overlayID].fileID]);
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

	// Generate FNT and FAT data so  it can be used for further calculations. Also for overlay ids
	uint32_t fntSize = mFS.CalculateFNTSize();
    uint32_t fatSize = mFS.CalculateFATSize();
    
    fntSize = PadTo32(fntSize);
    fatSize = PadTo32(fatSize);

    uint8_t* fntData = new uint8_t[fntSize];
    uint8_t* fatData = new uint8_t[fatSize];

    memset(fntData, 0, fntSize);
    memset(fatData, 0, fatSize);

    bStream::CMemoryStream fntStream(fntData, fntSize, bStream::Endianess::Little, bStream::OpenMode::Out);

    // calling write FNT will reset all IDs! it MUST be called FIRST
    mFS.WriteFNT(fntStream);

	std::vector<std::shared_ptr<File>> files = {};

	mFS.ForEachFile([&](std::shared_ptr<File> f){
		files.push_back(f);
	});
	
	std::sort(files.begin(), files.end(), [](std::shared_ptr<File> a, std::shared_ptr<File> b){ return a->GetID() < b->GetID(); });

	uint32_t overlayFirstFileID = files.back()->GetID(); // stupid hack. 

	for(int i = 0; i < mOverlays9.size(); i++){
		auto f = mOverlays9[i].file;
		f.lock()->SetID(overlayFirstFileID++);
		files.push_back(f.lock());
	}

	for(int i = 0; i < mOverlays7.size(); i++){
		auto f = mOverlays7[i].file;
		f.lock()->SetID(overlayFirstFileID++);
		files.push_back(f.lock());
	}

	// start on the rom specific stuff
	for(uint32_t i = 0; i < 0x4000; i++) romFile.writeUInt8(0x00);

	auto arm9 = mRomFiles->GetFile("arm9.bin");
	if(arm9){
		mHeader.arm9RomOff = romFile.tell();
		mHeader.arm9Size = arm9->GetSize();
		romFile.writeBytes(arm9->GetData(), arm9->GetSize());
	} else {
		mHeader.arm9RomOff = 0;
		mHeader.arm9Size = 0;
	}

	if(mNitroFooter[0] != 0x00000000){
		romFile.writeUInt32(mNitroFooter[0]);
		romFile.writeUInt32(mNitroFooter[1]);
		romFile.writeUInt32(mNitroFooter[2]);
	}

	std::vector<std::pair<uint32_t, uint32_t>> mOverlay9Fat = {};

	if(mOverlays9.size() > 0){
		while(romFile.tell() % 0x200 != 0) romFile.writeUInt8(0x00);

		mHeader.arm9OverlayOffset = romFile.tell();
		mHeader.arm9OverlaySize = mOverlays9.size() * 32;
		for(int i = 0; i < mOverlays9.size(); i++){
			romFile.writeUInt32(mOverlays9[i].overlayID);
			romFile.writeUInt32(mOverlays9[i].ramAddress);
			romFile.writeUInt32(mOverlays9[i].ramSize);
			romFile.writeUInt32(mOverlays9[i].bssSize);
			romFile.writeUInt32(mOverlays9[i].staticInitStart);
			romFile.writeUInt32(mOverlays9[i].staticInitEnd);
			romFile.writeUInt32(mOverlays9[i].file.lock()->GetID());
			romFile.writeUInt32(mOverlays9[i].compressedSize | (mOverlays9[i].flags << 24) | (0 << 25));
		}

		for(int i = 0; i < mOverlays9.size(); i++){
			mOverlay9Fat.push_back({romFile.tell(), 0});
			romFile.writeBytes(mOverlays9[i].file.lock()->GetData(), mOverlays9[i].file.lock()->GetSize());
			mOverlay9Fat.back().second = romFile.tell();
			while(romFile.tell() % 0x200 != 0) romFile.writeUInt8(0x00);
		}
		
		while(romFile.tell() % 0x200 != 0) romFile.writeUInt8(0x00);
	} else {
		mHeader.arm9OverlayOffset = 0;
		mHeader.arm9OverlaySize = 0;
	}

	while(romFile.tell() % 0x4000 != 0) romFile.writeUInt8(0x00);
	
	auto arm7 = mRomFiles->GetFile("arm7.bin");
	if(arm7){
		mHeader.arm7RomOff = romFile.tell();
		mHeader.arm7Size = arm7->GetSize();
		romFile.writeBytes(arm7->GetData(), arm7->GetSize());
	} else {
		mHeader.arm7RomOff = 0;
		mHeader.arm7Size = 0;
	}


	std::vector<std::pair<uint32_t, uint32_t>> mOverlay7Fat = {};

	if(mOverlays7.size() > 0){
		while(romFile.tell() % 0x200 != 0) romFile.writeUInt8(0x00);

		mHeader.arm7OverlayOffset = romFile.tell();
		mHeader.arm7OverlaySize = mOverlays7.size() * 32;
		for(int i = 0; i < mOverlays7.size(); i++){
			romFile.writeUInt32(mOverlays7[i].overlayID);
			romFile.writeUInt32(mOverlays7[i].ramAddress);
			romFile.writeUInt32(mOverlays7[i].ramSize);
			romFile.writeUInt32(mOverlays7[i].bssSize);
			romFile.writeUInt32(mOverlays7[i].staticInitStart);
			romFile.writeUInt32(mOverlays7[i].staticInitEnd);
			romFile.writeUInt32(mOverlays7[i].file.lock()->GetID());
			romFile.writeUInt32(mOverlays7[i].compressedSize | (mOverlays7[i].flags << 24) | (0 << 25));
		}

		for(int i = 0; i < mOverlays7.size(); i++){
			mOverlay7Fat.push_back({romFile.tell(), 0});
			romFile.writeBytes(mOverlays7[i].file.lock()->GetData(), mOverlays7[i].file.lock()->GetSize());
			mOverlay7Fat.back().second = romFile.tell();
			while(romFile.tell() % 0x200 != 0) romFile.writeUInt8(0x00);
		}
		
		while(romFile.tell() % 0x200 != 0) romFile.writeUInt8(0x00);
	} else {
		mHeader.arm7OverlayOffset = 0;
		mHeader.arm7OverlaySize = 0;
	}


	mHeader.FNTOffset = romFile.tell();
	mHeader.FNTSize = fntSize;
	romFile.writeBytes(fntData, fntSize);

	for(uint32_t i = 0; i < Pad(romFile.tell(), 0x200); i++) romFile.writeUInt8(0x00);

	mHeader.FATOffset = romFile.tell();
	mHeader.FATSize = files.size() * 0x08;

	for (int i=0;i<mHeader.FATSize;i++) romFile.writeUInt8(0xFF);

	while(romFile.tell() % 0x200 != 0) romFile.writeUInt8(0xFF);


	mHeader.iconBannerOffset = romFile.tell();
	romFile.writeBytes((uint8_t*)&mBanner, sizeof(mBanner));

	while(romFile.tell() % 0x200 != 0) romFile.writeUInt8(0x00);


	uint32_t dataOffset = romFile.tell();

	romFile.seek(mHeader.FATOffset);
	uint32_t startOffset = 0;
	for(int i = 0; i < files.size() - mOverlays9.size() - mOverlays7.size(); i++){
		romFile.writeUInt32(dataOffset + startOffset);
		romFile.writeUInt32(dataOffset + PadTo32(files[i]->GetSize()));
		startOffset += PadTo32(files[i]->GetSize());
	}
	
	for(int i = 0; i < mOverlays9.size(); i++){
		romFile.writeUInt32(mOverlay9Fat[i].first);
		romFile.writeUInt32(mOverlay9Fat[i].second);
	}

	for(int i = 0; i < mOverlays7.size(); i++){
		romFile.writeUInt32(mOverlay7Fat[i].first);
		romFile.writeUInt32(mOverlay7Fat[i].second);
	}

	romFile.seek(dataOffset);

	for(int i = 0; i < files.size() - mOverlays9.size() - mOverlays7.size(); i++){
		romFile.writeBytes(files[i]->GetData(), files[i]->GetSize());
	}

	//mHeader.totalUsedRom = romFile.tell();

	romFile.seek(0);
	romFile.writeBytes((uint8_t*)&mHeader, sizeof(mHeader));

    delete[] fntData;
    delete[] fatData;
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
