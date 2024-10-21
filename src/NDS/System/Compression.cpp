#include <NDS/System/Compression.hpp>

namespace Palkia::Nitro::Compression {

// Based on https://github.com/Barubary/dsdecmp/blob/master/CSharp/DSDecmp/Formats/LZOvl.cs
// thanksssss :3
void BLZDecompress(std::shared_ptr<File> target){
    bStream::CMemoryStream compressedStream(target->GetData(), target->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

    compressedStream.seek(target->GetSize()-0x04);
    uint32_t decompressedSize = compressedStream.readUInt32() + target->GetSize();
    
    if(decompressedSize == 0){
        return; // not compressedddddd
    }

    compressedStream.seek(target->GetSize()-0x08);
    uint8_t info = compressedStream.readUInt32();

    uint32_t compressedSize = (info & 0xFFFFFF);
    uint32_t srcOffset = (info >> 24);
    
    if(compressedSize + srcOffset == target->GetSize()){
        compressedSize = target->GetSize() - srcOffset;
    }
    
    std::vector<uint8_t> mDecompresedData;
    mDecompresedData.resize(target->GetSize() - srcOffset - compressedSize);

    

    compressedStream.seek(0);
    compressedStream.readBytesTo(mDecompresedData.data(), target->GetSize());



    target->SetData(mDecompresedData.data(), mDecompresedData.size());
}

void BLZCompress(std::shared_ptr<File> target){
    
}

}