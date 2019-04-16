#include <NitroRom.h>
#include <filesystem>
#include <functional>
#include "tests.h"



int main(){
    UnitTests tests;
    
    tests.registerTest("Load Rom", [](){
        bool passed = true;
        
        if(std::filesystem::exists(std::filesystem::path("test/files/platinum.nds"))){
            bStream::CFileStream stream("test/files/platinum.nds", bStream::OpenMode::In);
            NitroRom Platinum(stream);
        }
        

        return passed;
    });

    tests.runTests();
}