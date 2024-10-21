#include <NDS/System/Compression.hpp>

namespace Palkia::Nitro::Compression {

// Based on https://github.com/Barubary/dsdecmp/blob/master/CSharp/DSDecmp/Formats/LZOvl.cs
// thanksssss :3
void BLZDecompress(std::shared_ptr<File> target){
    bStream::CMemoryStream compressedStream(target->GetData(), target->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

    compressedStream.seek(target->GetSize()-0x08);
    uint32_t info = compressedStream.readUInt32();
    uint32_t extraSpace = compressedStream.readUInt32();

    if(extraSpace == 0){
        return; // not compressedddddd
    }


    uint32_t headerSize = (info >> 24);
    uint32_t compressedSize = (info & 0xFFFFFF);
    
    if(compressedSize >= target->GetSize()){
        compressedSize = target->GetSize();
    }
    
    std::vector<uint8_t> mDecompressedData;
    std::vector<uint8_t> mCompressedBuffer;
    std::vector<uint8_t> mUncompressedData;

    // grabs the uncompressed data at the start of the file
    mUncompressedData.resize(target->GetSize() - compressedSize, 0);
    compressedStream.seek(0);
    compressedStream.readBytesTo(mUncompressedData.data(), mUncompressedData.size());

    // output of decompressed data
    mDecompressedData.resize(target->GetSize() + extraSpace - mUncompressedData.size(), 0);

    // grabs the compressed data buffer
    mCompressedBuffer.resize(compressedSize, 0);
    compressedStream.seek(target->GetSize() - compressedSize - headerSize);
    compressedStream.readBytesTo(mCompressedBuffer.data(), mCompressedBuffer.size());

    uint32_t readBytes = 0, currentOutSize = 0;
    uint32_t decompressedSize = mDecompressedData.size();
    uint8_t mask = 1, flags = 0;
    while(currentOutSize < decompressedSize){
        if(mask == 1){
            if(readBytes >= compressedSize){
                std::cout << "[Palkia] BLZ ran out of data to decompress!" << std::endl;
                return; // fuck
            }

            flags = mCompressedBuffer[mCompressedBuffer.size() - 1 - readBytes];
            readBytes++;
            mask = 0x80;
        } else {
            mask >>= 1;
        }

        if((flags & mask) > 0){
            if(readBytes + 1 >= target->GetSize()){
                std::cout << "[Palkia] Ran out of data to decompress" << std::endl;
                return;
            }
            uint8_t a = mCompressedBuffer[mCompressedBuffer.size() - 1 - readBytes]; readBytes++;
            uint8_t b = mCompressedBuffer[mCompressedBuffer.size() - 1 - readBytes]; readBytes++;

            uint8_t len = (a >> 4) + 3;
            uint16_t disp = (((a & 0x0F) << 8) | b) + 3;

            if(disp > currentOutSize){
                if(currentOutSize < 2){
                    std::cout << "[Palkia] Invalid readback size!" << std::endl;
                    return;
                }
                disp = 2;
            }

            uint32_t bufIdx = currentOutSize - disp;
            for(uint32_t i = 0; i < len; i++){
                uint8_t next = mDecompressedData[decompressedSize - 1 - bufIdx];
                bufIdx++;
                mDecompressedData[decompressedSize - 1 - currentOutSize] = next;
                currentOutSize++;
            }
        } else {
            uint8_t next = mCompressedBuffer[compressedSize - 1 - readBytes];
            readBytes++;
            mDecompressedData[decompressedSize - 1 - currentOutSize] = next;
            currentOutSize++;
        }
    }

    std::vector<uint8_t> result;
    result.reserve(mDecompressedData.size() + mUncompressedData.size());
    result.insert(result.end(), mUncompressedData.begin(), mUncompressedData.end());
    std::cout << "[Palkia]: decompressed Data starts at 0x" << std::hex << result.size() << std::dec << std::endl;
    result.insert(result.end(), mDecompressedData.begin(), mDecompressedData.end());

    target->SetData(result.data(), result.size());
}

void BLZCompress(std::shared_ptr<File> target){
    
}

}