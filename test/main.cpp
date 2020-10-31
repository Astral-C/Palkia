#include <NitroRom.h>
#include <filesystem>
#include <functional>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "tests.h"

int main(){
    Tests tests;

    tests.registerTest("Load Rom, Dump Icon", [](){

        Palkia::NitroRom Platinum(std::filesystem::path("test/files/platinum.nds"));
        std::printf("Game Code is %s\n", Platinum.getHeader().gameCode);

        Palkia::Color iconBitmap[32][32];
        Platinum.getRawIcon(iconBitmap);

        stbi_write_png("icon.png", 32, 32, 4, (void*)iconBitmap, 4*32);

        return true;
    });

    tests.runTests();
}
