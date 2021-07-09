
#pragma once
#include <ntr/ntr_ROM.hpp>

namespace ntr::nfs {

    bool InitializeSelfNitroFs(const std::string &fat_path);
    std::shared_ptr<ROMFileHandle> CreateSelfNitroFsFileHandle();

}