
#pragma once
#include <ntr/fmt/fmt_ROM.hpp>

bool InitializeSelfNitroFs(const std::string &fat_path);
std::shared_ptr<ntr::fmt::ROMFileHandle> CreateSelfNitroFsFileHandle();