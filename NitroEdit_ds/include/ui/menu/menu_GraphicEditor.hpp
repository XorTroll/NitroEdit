
#pragma once
#include <ui/menu/menu_Base.hpp>

namespace ui::menu {

    void InitializeGraphicEditor();
    bool ShowGraphicEditor(ntr::gfx::abgr1555::Color *gfx_buf, ntr::gfx::abgr1555::Color *color_plt, const size_t color_plt_count, const u32 width, const u32 height);
    void UpdateGraphicEditorGraphics();

}