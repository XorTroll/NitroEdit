#include <ntr/fs/fs_Stdio.hpp>
#include <base_SelfNitroFs.hpp>

namespace {

    std::shared_ptr<ntr::fmt::ROM> g_SelfROM = {};
    bool g_Initialized = false;

}

bool InitializeSelfNitroFs(const std::string &fat_path) {
    if(g_Initialized) {
        return true;
    }

    g_SelfROM = std::make_shared<ntr::fmt::ROM>();
    if(g_SelfROM->ReadFrom(fat_path, std::make_shared<ntr::fs::StdioFileHandle>()).IsFailure()) {
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