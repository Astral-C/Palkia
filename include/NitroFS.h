#include <bstream.h>
#include <vector>

struct NitroDirEntry {
    uint32_t dirEntryStart;
    uint16_t dirEntryFileID;
    uint16_t dirParentID;
};


class NitroFNT {
public:
    void addDirectory(NitroDirEntry);
    NitroFNT(){}
    ~NitroFNT(){}

private:
    std::vector<NitroDirEntry> NitroDirEntries;
    
};