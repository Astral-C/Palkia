#include <vector>
#include <glad/glad.h>
#include <NDS/Assets/NSBTX.hpp>
#include <Util.hpp>
#include <string>
#include <fstream>
#include <algorithm>

namespace Palkia {

namespace TEX0 {

void Parse(bStream::CStream& stream, uint32_t offset, Nitro::ResourceDict<std::shared_ptr<Texture>>& textures, Nitro::ResourceDict<std::shared_ptr<Palette>>& palettes){
    stream.readUInt32(); // section size 0x04
    stream.skip(4); //0x08

    uint16_t textureDataSize = stream.readUInt16(); //0x0C
    uint16_t textureListOffset = stream.readUInt16(); //0x0E
    stream.skip(4);
    uint32_t textureDataOffset = stream.readUInt32(); //0x14
    stream.skip(4); //padding //0x18

    uint16_t cmpTexDataSize = stream.readUInt16() << 3; //0x1C
    uint32_t cmpTexInfoOffset = stream.readUInt16(); // 0x1E
    stream.skip(4); // padding 0x20
                
    uint32_t cmpTexDataOffset = stream.readUInt32(); //0x24
    uint32_t cmpTexInfoDataOffset = stream.readUInt32(); // 0x28 huh?
    stream.skip(4); // 0x2C

    uint32_t paletteDataSize = stream.readUInt32() << 3; //30
    uint32_t paletteDictOffset = stream.readUInt16(); //34
    stream.skip(2);
    uint32_t paletteDataOffset = stream.readUInt32(); //

    stream.seek(offset + paletteDictOffset);
    palettes = Nitro::ReadList<std::shared_ptr<Palette>>(stream, [&](bStream::CStream& stream){
        std::shared_ptr<Palette> palette = std::make_shared<Palette>(stream, paletteDataOffset + offset, paletteDataSize);
        return palette;
    });

    stream.seek(offset + textureListOffset);
    std::cout << "Reading Texture List at " << std::hex << offset << " " << textureListOffset << std::endl;
    textures = Nitro::ReadList<std::shared_ptr<Texture>>(stream, [&](bStream::CStream& stream){
        std::shared_ptr<Texture> texture = std::make_shared<Texture>(stream, textureDataOffset + offset);
        stream.readUInt32(); // wuh?
        return texture;
    });
}

Texture::Texture(bStream::CStream& stream, uint32_t texDataOffset){
    std::cout << "Reading Texture at " << std::hex << stream.tell() << std::endl;
    uint32_t params = stream.readUInt32();
    mFormat = (params >> 26) & 0x07;
    mWidth = 8 << ((params >> 20) & 0x07);
    mHeight = 8 << ((params >> 23) & 0x07);
    mColor0 = ((params >> 29) & 0x01);

    mDataOffset = (params & 0xFFFF) << 3;

    size_t pos = stream.tell();
    std::cout << "Reading texture at " << texDataOffset << " + " << mDataOffset << " = " << stream.tell() << std::endl;

    stream.seek(mDataOffset + texDataOffset);
    mImgData.resize(mWidth * mHeight);
    std::fill(mImgData.begin(), mImgData.end(), 0);

    if(mFormat == 0x03){
        for (size_t y = 0; y < mHeight; y++){
            for(size_t x = 0; x < mWidth; x+=4){
                uint16_t block = stream.readUInt16();
                for(size_t bx = 0; bx < 4; bx++){
                    uint16_t paletteIdx = block & 0x0F;
                    uint32_t dst = ((y * mWidth) + x + bx);
                    mImgData[dst] = paletteIdx;
                    block >>= 4;
                } 
            }
        }
    } else if(mFormat == 0x02){
        for (size_t y = 0; y < mHeight; y++){
            for(size_t x = 0; x < mWidth; x+=8){
                uint16_t block = stream.readUInt16();
                for(size_t bx = 0; bx < 4; bx++){
                    uint16_t paletteIdx = block & 0x03;
                    uint32_t dst = ((y * mWidth) + x + bx);
                    mImgData[dst] = paletteIdx;
                    block >>= 2;
                } 
            }
        }
    } else if(mFormat == 0x04){
        for (size_t y = 0; y < mHeight; y++){
            for(size_t x = 0; x < mWidth; x++){
                uint32_t dst = ((y * mWidth) + x);
                mImgData[dst] = stream.readUInt8();
            }
        }
    } else if(mFormat == 0x05){
        for (size_t by = 0; by < mHeight; by+=4){
            for(size_t bx = 0; bx < mWidth; bx+=4){
                
                uint32_t block = stream.readUInt32();
                uint16_t palBlock = stream.readUInt16();
            
                for (size_t y = 0; y < 4; y++){
                    for(size_t x = 0; x < 4; x++){
                        uint8_t colorIdx = block & 0x03;
                        uint32_t dst = ((by + y) * mWidth) + (bx + x);
                        mImgData[dst] = (colorIdx << 16) | palBlock;
                        block >>= 2; 
                    }
                }
            
            }
        }
    } else if(mFormat == 0x01){
        for (size_t y = 0; y < mHeight; y++){
            for(size_t x = 0; x < mWidth; x++){
                uint32_t dst = ((y * mWidth) + x);
                
                uint16_t block = stream.readUInt8();

                uint16_t paletteIdx = (block & 0x1F) << 1;
                uint16_t alpha = cv3To8(block >> 5);
                
                mImgData[dst] = (paletteIdx << 16) | alpha;
            }
        }
    } else if(mFormat == 0x06){
        for (size_t y = 0; y < mHeight; y++){
            for(size_t x = 0; x < mWidth; x++){
                uint32_t dst = ((y * mWidth) + x);
                
                uint16_t block = stream.readUInt8();

                uint16_t paletteIdx = (block & 0x07) << 1;
                uint16_t alpha = cv3To8(block >> 3);
                
                mImgData[dst] = (paletteIdx << 16) | alpha;
            }
        }
    } else if(mFormat == 0x07){
        for (size_t y = 0; y < mHeight; y++){
            for(size_t x = 0; x < mWidth; x++){
                uint32_t dst = ((y * mWidth) + x);
                mImgData[dst] = stream.readUInt16();
            }
        }
    }

    stream.seek(pos);
}

uint32_t Texture::Convert(Palette p){
    uint32_t mTexture = 0;
    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    std::vector<uint8_t> image;
    image.resize(mWidth * mHeight * 4);

    for (size_t y = 0; y < mHeight; y++){
        for(size_t x = 0; x < mWidth; x++){
            uint32_t src = ((y * mWidth) + x);
            uint32_t dst = ((y * mWidth) + x) * 4;
            if(mFormat == 0x02 || mFormat == 0x03 || mFormat == 0x04){
                image[dst]   = p.GetColors()[mImgData[src]].r;
                image[dst+1] = p.GetColors()[mImgData[src]].g;
                image[dst+2] = p.GetColors()[mImgData[src]].b;
                image[dst+3] = mImgData[src] == 0 ? (mColor0 ? 0x00 : 0xFF) : 0xFF;
            } else if(mFormat == 0x01 || mFormat == 0x06){
                image[dst]   = p.GetColors()[(mImgData[src] & 0xFFFF) >> 16].r;
                image[dst+1] = p.GetColors()[(mImgData[src] & 0xFFFF) >> 16].g;
                image[dst+2] = p.GetColors()[(mImgData[src] & 0xFFFF) >> 16].b;
                image[dst+3] = mImgData[src] & 0x0000FFFF;
            } else if(mFormat == 0x05){
                // horrible, does this for each pixel!
                glm::vec4 color = p.FromColorTable(mImgData[src] & 0x0000FFFF, mImgData[src]);

                image[dst]   = color.r;
                image[dst+1] = color.g;
                image[dst+2] = color.b;
                image[dst+3] = color.a;
            } else if(mFormat == 0x07){
                image[dst]   = cv5To8(mImgData[src] & 0x1F);
                image[dst+1] = cv5To8((mImgData[src] >> 5) & 0x1F);
                image[dst+2] = cv5To8((mImgData[src] >> 10) & 0x1F);
                image[dst+3] = 0xFF;
            }
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    return mTexture;

}

Palette::Palette(bStream::CStream& stream, uint32_t paletteDataOffset, uint32_t paletteDataSize){
    uint32_t paletteOffset = (stream.readUInt16() << 3) + paletteDataOffset;
    stream.skip(2);

    size_t listPos = stream.tell();
    
    size_t tempPos = listPos;
    uint32_t nextPaletteOffset = (stream.peekUInt16(tempPos) << 3) + paletteDataOffset;
    while(nextPaletteOffset == paletteOffset){
        nextPaletteOffset = (stream.peekUInt16(tempPos+=2) << 3) + paletteDataOffset;
    }

    if(nextPaletteOffset > paletteDataOffset + paletteDataSize || nextPaletteOffset < paletteOffset){
        nextPaletteOffset = paletteDataOffset + paletteDataSize;
    }


    mColorCount = (nextPaletteOffset - paletteOffset) >> 1;
    std::cout << "Current Palette Offset is 0x" << std::hex << paletteOffset << " Next Palette Offset is 0x" << nextPaletteOffset << " color count is " << mColorCount << std::endl; 
    stream.seek(paletteOffset);

    for(size_t i = 0; i < mColorCount; i++){
        glm::vec3 color;
        uint16_t pentry = stream.readUInt16();
        color.r = cv5To8(pentry & 0x1F);
        color.g = cv5To8((pentry >> 5) & 0x1F);
        color.b = cv5To8((pentry >> 10) & 0x1F);
        mColors.push_back(color);
    }

    stream.seek(listPos);

}

glm::vec4 Palette::FromColorTable(uint16_t palBlock, uint16_t colorIdx){

    if(!mColorTables.contains(palBlock)){
        std::array<uint8_t, 16> colorTable;

        uint16_t mode = palBlock >> 14;
        uint16_t idx = ((palBlock & 0x3FFF) << 2) >> 2;

        colorTable[0] = mColors[idx].r;
        colorTable[1] = mColors[idx].g;
        colorTable[2] = mColors[idx].b;
        colorTable[3] = 0xFF;

        colorTable[4] = mColors[idx + 1].r;
        colorTable[5] = mColors[idx + 1].g;
        colorTable[6] = mColors[idx + 1].b;
        colorTable[7] = 0xFF;

        if(mode == 0){
            colorTable[8] = mColors[idx + 2].r;
            colorTable[9] = mColors[idx + 2].g;
            colorTable[10] = mColors[idx + 2].b;
            colorTable[11] = 0xFF;
        } else if(mode == 1){
            colorTable[8] = (colorTable[0] + colorTable[4]) >> 1;
            colorTable[9] = (colorTable[1] + colorTable[5]) >> 1;
            colorTable[10] = (colorTable[2] + colorTable[6]) >> 1;
            colorTable[11] = 0xFF;
        } else if(mode == 2){
            colorTable[8] = mColors[idx + 2].r;
            colorTable[9] = mColors[idx + 2].g;
            colorTable[10] = mColors[idx + 2].b;
            colorTable[11] = 0xFF;

            colorTable[12] = mColors[idx + 3].r;
            colorTable[13] = mColors[idx + 3].g;
            colorTable[14] = mColors[idx + 3].b;
            colorTable[15] = 0xFF;
        } else {
            colorTable[8] = s3tcBlend(colorTable[4], colorTable[0]);
            colorTable[9] = s3tcBlend(colorTable[5], colorTable[1]);
            colorTable[10] = s3tcBlend(colorTable[6], colorTable[2]);
            colorTable[11] = 0xFF;

            colorTable[12] = s3tcBlend(colorTable[0], colorTable[4]);
            colorTable[13] = s3tcBlend(colorTable[1], colorTable[5]);
            colorTable[14] = s3tcBlend(colorTable[2], colorTable[6]);
            colorTable[15] = 0xff;
        }

        mColorTables[palBlock] = colorTable;
    }

    glm::vec4 color;
    
    color.r = mColorTables[palBlock][(colorIdx * 4) + 0];
    color.g = mColorTables[palBlock][(colorIdx * 4) + 1];
    color.b = mColorTables[palBlock][(colorIdx * 4) + 2];
    color.a = mColorTables[palBlock][(colorIdx * 4) + 3];
    
    return color;
}

}

namespace Formats {

void NSBTX::Load(bStream::CStream& stream){
    stream.readUInt32(); // stamp
    stream.readUInt16(); // byte order

    stream.readUInt16(); // ver
    stream.readUInt32(); // filesize

    stream.readUInt16(); // header size

    uint16_t sectionCount = stream.readUInt16();

    for (size_t i = 0; i < sectionCount; i++){
        uint32_t sectionOffset = stream.readUInt32();
        uint32_t returnOffset = stream.tell();
        stream.seek(sectionOffset);

        std::cout << "Reading Segment at " << std::hex << stream.tell() << std::endl;
        std::cout << "Stamp is " << stream.peekString(stream.tell(), 4) << std::endl;
        uint32_t stamp = stream.readUInt32();

        switch (stamp){
            case 0x30584554: { // TEX0 
                TEX0::Parse(stream, sectionOffset, mTextures, mPalettes);
                break;   
            }

            default:
                break;
        }
    
    
        stream.seek(returnOffset);

    }


}

}
}