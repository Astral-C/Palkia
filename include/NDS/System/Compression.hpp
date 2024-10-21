#pragma once
#include <NDS/System/FileSystem.hpp>

namespace Palkia::Nitro::Compression {

    void BLZDecompress(std::shared_ptr<File> target);
    void BLZCompress(std::shared_ptr<File> target);

}