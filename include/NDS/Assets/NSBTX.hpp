#pragma once
#include <Util.hpp>
#include <bstream/bstream.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <map>
#include <array>

namespace Palkia {
namespace TEX0 {

    class Palette { 
        uint32_t mColorCount { 0 };
        std::vector<glm::vec3> mColors;
        std::map<uint16_t, std::array<uint8_t, 16>> mColorTables;
    public: 
        glm::vec4 FromColorTable(uint16_t idx, uint16_t colorIdx);
        std::vector<glm::vec3> GetColors() { return mColors; }
        Palette(bStream::CStream&, uint32_t, uint32_t);
        Palette(){}
        ~Palette(){}
    };

    class Texture {
        uint32_t mFormat;
        uint32_t mWidth, mHeight;
        uint32_t mColor0;
        uint32_t mDataOffset;

        std::vector<uint32_t> mImgData;


    public:
    
        uint32_t GetFormat() { return mFormat; }
        uint32_t GetWidth() { return mWidth; }
        uint32_t GetHeight() { return mHeight; }
        Texture(bStream::CStream&, uint32_t);

        std::vector<uint8_t> Convert(Palette p);
        
        void Bind();

        Texture(){}
        ~Texture(){}

    };

    void Parse(bStream::CStream& stream, uint32_t offset, Nitro::ResourceDict<std::shared_ptr<Texture>>& textures, Nitro::ResourceDict<std::shared_ptr<Palette>>& palettes);
}

namespace Formats {

class NSBTX {
    Nitro::ResourceDict<std::shared_ptr<TEX0::Texture>> mTextures;
    Nitro::ResourceDict<std::shared_ptr<TEX0::Palette>> mPalettes;
public:
    Nitro::ResourceDict<std::shared_ptr<TEX0::Texture>>& GetTextures() { return mTextures; }
    Nitro::ResourceDict<std::shared_ptr<TEX0::Palette>>& GetPalettes() { return mPalettes; }

    void Load(bStream::CStream& stream);
    
    NSBTX(){}
    ~NSBTX(){}
};

}

}