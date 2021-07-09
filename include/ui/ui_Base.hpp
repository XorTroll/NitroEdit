

#pragma once
#include <gfx/gfx_Base.hpp>
#include <fs/fs_BinaryFile.hpp>

namespace ui {

    constexpr u32 ScreenWidth = 256;
    constexpr u32 ScreenHeight = 192;

    bool LoadPNGGraphics(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, u32 &out_width, u32 &out_height, gfx::abgr1555::Color *&out_gfx_buf);

}