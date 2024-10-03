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

		romFile.seek(mHeader.arm9RomOff);
		uint8_t* arm9Data = new uint8_t[mHeader.arm9Size];
		romFile.readBytesTo(arm9Data, mHeader.arm9Size);

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
		arm9->SetName("arm7.bin");
		arm9->SetData(arm7Data, mHeader.arm7Size);

		auto debugRom = File::Create();
		debugRom->SetName("debug.nds");
		debugRom->SetData(debugRomData, mHeader.debugRomSize);

		mRomFiles->AddFile(arm9);
		mRomFiles->AddFile(arm7);
		mRomFiles->AddFile(debugRom);

		if(mHeader.arm9OverlayOffset != 0){
			romFile.seek(mHeader.arm9OverlayOffset);
			mOverlays9.resize(mHeader.arm9OverlaySize / 32);
			for(int i = 0; i < mHeader.arm9OverlaySize; i += 32){
				uint32_t overloayID = romFile.readUInt32();
				mOverlays9[overloayID].ramAddress = romFile.readUInt32();
				mOverlays9[overloayID].ramSize = romFile.readUInt32();
				mOverlays9[overloayID].bssSize = romFile.readUInt32();
				mOverlays9[overloayID].staticInitStart = romFile.readUInt32();
				mOverlays9[overloayID].staticInitEnd = romFile.readUInt32();
				uint32_t fileID = romFile.readUInt32();
				uint32_t compressedFlags = romFile.readUInt32();
				mOverlays9[overloayID].compressedSize = compressedFlags & 0xFFFFFF;
				mOverlays9[overloayID].flags = compressedFlags >> 24;

				auto file = files.at(fileID);
				file->SetName(std::format("overlay_{}.bin", overloayID));
				overlay9Dir->AddFile(file);
				mOverlays9[overloayID].file = file;
			}
		}

		if(mHeader.arm7OverlayOffset != 0){
			romFile.seek(mHeader.arm7OverlayOffset);
			mOverlays7.resize(mHeader.arm7OverlaySize / 32);
			for(int i = 0; i < mHeader.arm7OverlaySize; i += 32){
				uint32_t overloayID = romFile.readUInt32();
				mOverlays7[overloayID].ramAddress = romFile.readUInt32();
				mOverlays7[overloayID].ramSize = romFile.readUInt32();
				mOverlays7[overloayID].bssSize = romFile.readUInt32();
				mOverlays7[overloayID].staticInitStart = romFile.readUInt32();
				mOverlays7[overloayID].staticInitEnd = romFile.readUInt32();
				uint32_t fileID = romFile.readUInt32();
				uint32_t compressedFlags = romFile.readUInt32();
				mOverlays7[overloayID].compressedSize = compressedFlags & 0xFFFFFF;
				mOverlays7[overloayID].flags = compressedFlags >> 24;

				auto file = files.at(fileID);
				file->SetName(std::format("overlay_{}.bin", overloayID));
				overlay7Dir->AddFile(file);
				mOverlays7[overloayID].file = file;
			}
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

	uint32_t headerSize = Pad(sizeof(RomHeader), 0x4000);
	for(uint32_t i = 0; i < headerSize; i++) romFile.writeUInt8(0xFF);


	mHeader.arm9RomOff = romFile.tell();
	auto arm9 = mRomFiles->GetFile("arm9.bin");
	mHeader.arm9Size = arm9->GetSize();
	romFile.writeBytes(arm9->GetData(), arm9->GetSize());
	for(uint32_t i = 0; i < Pad(romFile.tell(), 0x200); i++) romFile.writeUInt8(0xFF);

	mHeader.arm9OverlayOffset = romFile.tell();

	// write overlay9 table
	for(int i = 0; i < mOverlays9.size(); i++){
		romFile.writeUInt32(i);
		romFile.writeUInt32(mOverlays9[i].ramAddress);
		romFile.writeUInt32(mOverlays9[i].ramSize);
		romFile.writeUInt32(mOverlays9[i].bssSize);
		romFile.writeUInt32(mOverlays9[i].staticInitStart);
		romFile.writeUInt32(mOverlays9[i].staticInitEnd);
		romFile.writeUInt32(mOverlays9[i].file.lock()->GetID());
		romFile.writeUInt32((mOverlays9[i].flags << 24) | (mOverlays9[i].compressedSize & 0xFFFFFF));
	}

	mHeader.arm9OverlaySize = romFile.tell() - mHeader.arm9OverlayOffset;

	for(uint32_t i = 0; i < Pad(romFile.tell(), 0x200); i++) romFile.writeUInt8(0xFF);

	// write overlays
	for(int i = 0; i < mOverlays9.size(); i++){
		romFile.writeBytes(mOverlays9[i].file.lock()->GetData(),mOverlays9[i].file.lock()->GetSize());
	}


	for(uint32_t i = 0; i < Pad(romFile.tell(), 0x200); i++) romFile.writeUInt8(0xFF);


	mHeader.arm7RomOff = romFile.tell();
	auto arm7 = mRomFiles->GetFile("arm7.bin");
	mHeader.arm7Size = arm7->GetSize();
	romFile.writeBytes(arm7->GetData(), arm7->GetSize());
	for(uint32_t i = 0; i < Pad(romFile.tell(), 0x200); i++) romFile.writeUInt8(0xFF);

	mHeader.arm7OverlayOffset = romFile.tell();

	// write overlay7 table
	for(int i = 0; i < mOverlays7.size(); i++){
		romFile.writeUInt32(i);
		romFile.writeUInt32(mOverlays7[i].ramAddress);
		romFile.writeUInt32(mOverlays7[i].ramSize);
		romFile.writeUInt32(mOverlays7[i].bssSize);
		romFile.writeUInt32(mOverlays7[i].staticInitStart);
		romFile.writeUInt32(mOverlays7[i].staticInitEnd);
		romFile.writeUInt32(mOverlays7[i].file.lock()->GetID());
		romFile.writeUInt32((mOverlays7[i].flags << 24) | (mOverlays7[i].compressedSize & 0xFFFFFF));
	}

	mHeader.arm7OverlaySize = romFile.tell() - mHeader.arm7OverlayOffset;

	for(uint32_t i = 0; i < Pad(romFile.tell(), 0x200); i++) romFile.writeUInt8(0xFF);

	// write overlays
	for(int i = 0; i < mOverlays9.size(); i++){
		romFile.writeBytes(mOverlays9[i].file.lock()->GetData(),mOverlays9[i].file.lock()->GetSize());
	}

	for(uint32_t i = 0; i < Pad(romFile.tell(), 0x200); i++) romFile.writeUInt8(0xFF);

	mHeader.FNTOffset = romFile.tell();
	mHeader.FNTSize = fntSize;

    // Write FNT header
    romFile.writeUInt32(0x464E5442);
    romFile.writeUInt32(fntSize + 0x08);
    romFile.writeBytes(fntData, fntSize);

	for(uint32_t i = 0; i < Pad(romFile.tell(), 0x200); i++) romFile.writeUInt8(0xFF);

	romFile.writeUInt32(0x46415442);
    romFile.writeUInt32(fatSize + 0x0C);
    romFile.writeUInt32(files.size());
    romFile.writeBytes(fatData, fatSize);

	mHeader.FATOffset = romFile.tell();
	mHeader.FATSize = fatSize;

	for(uint32_t i = 0; i < Pad(romFile.tell(), 0x200); i++) romFile.writeUInt8(0xFF);

	mHeader.iconBannerOffset = romFile.tell();
	romFile.writeBytes((uint8_t*)&mBanner, sizeof(mBanner));

	for(uint32_t i = 0; i < Pad(romFile.tell(), 0x200); i++) romFile.writeUInt8(0xFF);

	mHeader.debugRomOffset = romFile.tell();
	auto debugRom = mRomFiles->GetFile("debug.nds");
	mHeader.debugRomSize = debugRom->GetSize();
	romFile.writeBytes(debugRom->GetData(), debugRom->GetSize());
	for(uint32_t i = 0; i < Pad(romFile.tell(), 0x200); i++) romFile.writeUInt8(0xFF);

	
	uint32_t fileOffset = romFile.tell();
	for(uint32_t i = 0; i < files.size(); i++){
		auto file = files.at(i);
		romFile.writeBytes(file->GetData(), PadTo32(file->GetSize()));	
	}

	for(uint32_t i = 0; i < Pad(romFile.tell(), 0x400); i++) romFile.writeUInt8(0xFF);

	romFile.seek(mHeader.FATOffset);
	mFS.WriteFAT(romFile, fileOffset);

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
