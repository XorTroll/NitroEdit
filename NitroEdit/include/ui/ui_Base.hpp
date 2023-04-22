
#pragma once
#include <base_Include.hpp>
#include <ntr/gfx/gfx_Base.hpp>
#include <ntr/fs/fs_BinaryFile.hpp>

namespace ui {

    constexpr u32 ScreenWidth = 256;
    constexpr u32 ScreenHeight = 192;

    bool LoadPNGGraphics(const std::string &path, std::shared_ptr<ntr::fs::FileHandle> file_handle, u32 &out_width, u32 &out_height, ntr::gfx::abgr1555::Color *&out_gfx_buf);

}