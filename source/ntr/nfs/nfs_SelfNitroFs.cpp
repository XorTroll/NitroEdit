#include <ntr/nfs/nfs_SelfNitroFs.hpp>
#include <fs/fs_Fat.hpp>

namespace ntr::nfs {

    namespace {

        ROM g_Self = {};
        bool g_Initialized = false;

    }

    bool InitializeSelfNitroFs(const std::string &fat_path) {
        if(g_Initialized) {
            return true;
        }

        if(!g_Self.ReadFrom(fat_path, std::make_shared<fs::FatFileHandle>())) {
            return false;
        }

        g_Initialized = true;
        return true;
    }

    std::shared_ptr<ROMFileHandle> CreateSelfNitroFsFileHandle() {
        if(g_Initialized) {
            return std::make_shared<ROMFileHandle>(g_Self);
        }
        return nullptr;
    }

}