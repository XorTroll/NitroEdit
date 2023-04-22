
#pragma once
#include <ui/ui_Base.hpp>

namespace ui {

    struct Background {
        int id;
        bool is_sub;
        u32 width;
        u32 height;
        ntr::gfx::abgr1555::Color *gfx_ptr;
        bool owns_gfx_ptr;

        Background() : id(0), is_sub(false), width(0), height(0), gfx_ptr(nullptr), owns_gfx_ptr(false) {}

        ~Background() {
            this->Unload();
        }

        void Create(const bool is_sub) {
            this->Unload();
            this->is_sub = is_sub;

            if(is_sub) {
                this->id = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
            }
            else {
                this->id = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
            }
        }

        void Unload() {
            if(this->gfx_ptr != nullptr) {
                if(this->owns_gfx_ptr) {
                    delete[] this->gfx_ptr;
                    this->owns_gfx_ptr = false;
                }
                this->gfx_ptr = nullptr;
            }
        }

        bool LoadFrom(const std::string &path, std::shared_ptr<ntr::fs::FileHandle> file_handle) {
            this->Unload();
            if(!LoadPNGGraphics(path, file_handle, this->width, this->height, this->gfx_ptr)) {
                return false;
            }
            this->owns_gfx_ptr = true;
            return true;
        }

        bool LoadFrom(ntr::gfx::abgr1555::Color *gfx_buf, const u32 width, const u32 height, const bool owns_gfx_buf = false) {
            this->Unload();
            this->gfx_ptr = gfx_buf;
            this->width = width;
            this->height = height;
            this->owns_gfx_ptr = owns_gfx_buf;
            return true;
        }

        void Refresh() {
            if(this->gfx_ptr != nullptr) {
                auto bg_gfx_mem = reinterpret_cast<ntr::gfx::abgr1555::Color*>(bgGetGfxPtr(this->id));
                dmaCopy(this->gfx_ptr, bg_gfx_mem, this->width * this->height * sizeof(ntr::gfx::abgr1555::Color));
            }
        }

        template<typename C>
        inline void SetPixel(const u32 x, const u32 y, const C clr) {
            auto bg_gfx_mem = reinterpret_cast<ntr::gfx::abgr1555::Color*>(bgGetGfxPtr(this->id));
            const auto raw_val = ntr::gfx::ConvertColor<C, ntr::gfx::abgr1555::Color>(clr).raw_val;
            this->gfx_ptr[y * this->width + x].raw_val = raw_val;
            bg_gfx_mem[y * this->width + x].raw_val = raw_val;
        }
    };

}