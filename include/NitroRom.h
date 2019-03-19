#include <string>
#include <filesystem>

class PokeRom {
    public:
        PokeRom(std::filesystem::path);
        ~PokeRom(){}
    private:
        std::string rom;

};