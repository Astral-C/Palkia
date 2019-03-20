#include <functional>
#include <map>

class UnitTests {
    private:
        std::map<std::string, std::function<bool()>> tests;
    public:
        void registerTest(std::string s, std::function<bool()> f){
            tests.insert(std::pair(s, f));
        }

        void runTests(){
            for(auto& [name, func] : tests){
                std::printf("\033[1;36m==Running Test %s==\033[0m\n", name.c_str());
                bool res = func();
                std::printf((res ? "\033[1;32mPassed Test %s!\033[0m\n" : "\033[1;31mFailed Test %s!\033[0m\n"), name.c_str());
            }
        }
};