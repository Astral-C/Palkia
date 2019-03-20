#include <NitroRom.h>
#include <filesystem>
#include <functional>
#include "tests.h"



int main(){
    UnitTests tests;
    
    tests.registerTest("Load Rom", [](){
        bool passed = true;
        
        NitroRom Platinum(std::filesystem::path("test/files/platinum.nds"));
        std::printf("Game Code is %s\n", Platinum.getHeader().gameCode);

        return passed;
    });

    tests.runTests();
}