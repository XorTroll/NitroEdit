#include <ntr/fs/fs_Stdio.hpp>
#include <base_SelfNitroFs.hpp>

namespace {

    ntr::fmt::ROM g_SelfROM = {};
    bool g_Initialized = false;

}

bool InitializeSelfNitroFs(const std::string &fat_path) {
    if(g_Initialized) {
        return true;
    }

    if(!g_SelfROM.ReadFrom(fat_path, std::make_shared<ntr::fs::StdioFileHandle>())) {
        return false;
    }

    g_Initialized = true;
    return true;
}

std::shared_ptr<ntr::fmt::ROMFileHandle> CreateSelfNitroFsFileHandle() {
    if(g_Initialized) {
        return std::make_shared<ntr::fmt::ROMFileHandle>(g_SelfROM);
    }
    return nullptr;
}