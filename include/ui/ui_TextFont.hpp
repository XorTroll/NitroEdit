
#pragma once
#include <ui/ui_Sprite.hpp>
#include <extra/stb_truetype.h>

namespace ui {

    struct TextFont {
        stbtt_fontinfo font;
        u8 *ttf_buf;

        TextFont() : ttf_buf(nullptr) {}

        ~TextFont() {
            this->Dispose();
        }

        bool LoadFrom(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle);
    
        bool RenderTextBase(const std::string &text, const u32 x, const u32 y, const u32 line_height, const u32 max_line_width, const bool is_sub, const gfx::abgr1555::Color clr, const bool centered, Sprite &out_spr);

        template<typename C>
        inline bool RenderText(const std::string &text, const u32 x, const u32 y, const u32 line_height, const u32 max_line_width, const bool is_sub, const C clr, const bool centered, Sprite &out_spr) {
            return this->RenderTextBase(text, x, y, line_height, max_line_width, is_sub, gfx::ConvertColor<C, gfx::abgr1555::Color>(clr), centered, out_spr);
        }

        void Dispose() {
            if(this->ttf_buf != nullptr) {
                delete[] this->ttf_buf;
                this->ttf_buf = nullptr;
            }
        }
    };

}