#include <bstream.h>

struct NitroDirEntry {
    uint32_t dirEntryStart;
    uint16_t dirEntryFileID;
    uint16_t dirParentID;
};

NitroDirEntry ReadDirEntry(bStream::CStream* r){
    NitroDirEntry entry;
    entry.dirEntryStart = r->readUInt32();
    entry.dirEntryFileID = r->readUInt16();
    entry.dirParentID = r->readUInt16();
    return entry;
}
