#include <ui/ui_TextFont.hpp>

namespace ui {

    namespace {

        struct CodepointBitmap {
            u8 *bmp;
            u32 base_x;
            u32 base_y;
            u32 width;
            u32 height;
            bool center_applied;
        };

        inline void HandleCenteredLineImpl(const bool final, std::vector<CodepointBitmap> &bmps, const u32 max_line_width, const u32 cur_width, const u32 max_width) {
            const auto new_base_x = (max_line_width - cur_width) / 2;
            for(auto &prev_bmp : bmps) {
                if(!prev_bmp.center_applied) {
                    prev_bmp.base_x += new_base_x;
                    prev_bmp.center_applied = true;
                }
                if(final) {
                    prev_bmp.base_x -= (max_line_width - max_width) / 2;
                }
            }
        }

    }

    bool TextFont::LoadFrom(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle) {
        fs::BinaryFile ttf_bf;
        if(!ttf_bf.Open(file_handle, path, fs::OpenMode::Read)) {
            return false;
        }
        const auto ttf_size = ttf_bf.GetSize();
        this->ttf_buf = util::NewArray<u8>(ttf_size);
        if(!ttf_bf.ReadData(this->ttf_buf, ttf_size)) {
            return false;
        }
        ttf_bf.Close();

        if(!stbtt_InitFont(&this->font, this->ttf_buf, 0)) {
            return false;
        }
        return true;
    }

    bool TextFont::RenderTextBase(const std::string &text, const u32 x, const u32 y, const u32 line_height, const u32 max_line_width, const bool is_sub, const gfx::abgr1555::Color clr, const bool centered, Sprite &out_text) {
        std::vector<CodepointBitmap> bmps;
        
        u32 max_width = 0;
        auto max_height = line_height;

        u32 cur_width = 0;
        u32 cur_base_y = 0;

        const auto scale = stbtt_ScaleForPixelHeight(&this->font, line_height);
        
        int ascent;
        int descent;
        int line_gap;
        stbtt_GetFontVMetrics(&this->font, &ascent, &descent, &line_gap);
        ascent = roundf(ascent * scale);
        descent = roundf(descent * scale);
        
        for(u32 i = 0; i < text.length(); i++) {
            int ax;
            int lsb;
            stbtt_GetCodepointHMetrics(&this->font, text[i], &ax, &lsb);

            int c_x1;
            int c_y1;
            int c_x2;
            int c_y2;
            stbtt_GetCodepointBitmapBox(&this->font, text[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
            auto by = static_cast<u32>(ascent + c_y1);
            
            int out_w;
            int out_h;
            int xoff;
            int yoff;
            auto bmp = stbtt_GetCodepointBitmap(&this->font, scale, scale, text[i], &out_w, &out_h, &xoff, &yoff);
            const auto out_h_c = static_cast<u32>(out_h);
            if(max_height < out_h_c) {
                max_height = out_h_c;
            }

            CodepointBitmap c_bmp = {
                .bmp = bmp,
                .base_x = cur_width,
                .base_y = by + cur_base_y,
                .width = static_cast<u32>(out_w),
                .height = out_h_c,
                .center_applied = false
            };

            const auto r_ax = roundf(ax * scale);
            cur_width += r_ax;
            const auto kern = stbtt_GetCodepointKernAdvance(&this->font, text[i], text[i + 1]);
            const auto r_kern = roundf(kern * scale);
            cur_width += r_kern;
            if((cur_width > max_line_width) || (text[i] == '\n') || (text[i] == '\r')) {
                max_height += line_height;
                cur_base_y += line_height;
                if((text[i] == '\n') || (text[i] == '\r')) {
                    if(centered) {
                        HandleCenteredLineImpl(false, bmps, max_line_width, cur_width, max_width);
                    }
                    cur_width = 0;
                    continue;
                }
                else {
                    cur_width = r_ax;
                    c_bmp.base_x = 0;
                    c_bmp.base_y = by + cur_base_y;
                }
                
            }
            if(max_width < cur_width) {
                max_width = cur_width;
            }

            bmps.push_back(std::move(c_bmp));
        }

        if(centered) {
            HandleCenteredLineImpl(true, bmps, max_line_width, cur_width, max_width);
        }

        auto gfx_buf = util::NewArray<gfx::abgr1555::Color>(max_width * max_height);
        for(const auto &bmp : bmps) {
            for(u32 ry = 0; ry < bmp.height; ry++) {
                for(u32 rx = 0; rx < bmp.width; rx++) {
                    const auto val = bmp.bmp[ry * bmp.width + rx];
                    if(val > 0x10) {
                        const auto dst_x = bmp.base_x + rx;
                        const auto dst_y = bmp.base_y + ry;
                        gfx_buf[dst_y * max_width + dst_x].raw_val = clr.raw_val;
                    }
                }
            }
            stbtt_FreeBitmap(bmp.bmp, nullptr);
        }

        return out_text.CreateFrom(is_sub, x, y, gfx_buf, max_width, max_height, true);
    }

}