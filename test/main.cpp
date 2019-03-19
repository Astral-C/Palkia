#include <NitroRom.h>
#include <filesystem>

int main(){
    PokeRom Pearl(std::filesystem::path("test/files/platinum.nds"));
}