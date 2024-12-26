
#pragma once
#include <ui/ui_Base.hpp>

namespace ui {

    struct SpriteData {
        u32 idx;
        ntr::gfx::abgr1555::Color *oam_gfx_ptr;
    };

    constexpr SpriteSize BaseSpriteSize = SpriteSize_32x32;
    constexpr u32 BaseSpriteSizeValue = 32;

    struct Sprite {
        ntr::gfx::abgr1555::Color *gfx_buf;
        bool owns_gfx_buf;
        SpriteData *sprites;
        u32 sprite_w;
        u32 sprite_h;
        bool is_sub;
        int x;
        int y;
        u32 width;
        u32 height;
        bool visible;

        Sprite() {
            this->Reset();
        }

        Sprite(Sprite&& other) {
            this->gfx_buf = other.gfx_buf;
            this->owns_gfx_buf = other.owns_gfx_buf;
            this->sprites = other.sprites;
            this->sprite_w = other.sprite_w;
            this->sprite_h = other.sprite_h;
            this->is_sub = other.is_sub;
            this->x = other.x;
            this->y = other.y;
            this->width = other.width;
            this->height = other.height;
            this->visible = other.visible;
            other.Reset();
        }

        ~Sprite() {
            this->Dispose();
        }

        void Reset() {
            this->gfx_buf = nullptr;
            this->owns_gfx_buf = false;
            this->sprites = nullptr;
            this->sprite_w = 0;
            this->sprite_h = 0;
            this->is_sub = false;
            this->x = 0;
            this->y = 0;
            this->width = 0;
            this->height = 0;
            this->visible = true;
        }

        ntr::Result Load();
        void Update() const;

        inline void Refresh() {
            for(u32 i = 0; i < this->width * this->height; i++) {
                const auto x = i % this->width;
                const auto y = i / this->width;
                this->SetPixel(x, y, this->gfx_buf[i]);
            }
        }
        
        ntr::Result CreateFrom(const bool is_sub, const int x, const int y, const std::string &path, std::shared_ptr<ntr::fs::FileHandle> file_handle) {
            this->Dispose();
            NTR_R_TRY(LoadPNGGraphics(path, file_handle, this->width, this->height, this->gfx_buf));

            this->owns_gfx_buf = true;
            this->is_sub = is_sub;
            this->x = x;
            this->y = y;
            NTR_R_SUCCEED();
        }

        ntr::Result CreateFrom(const bool is_sub, const int x, const int y, ntr::gfx::abgr1555::Color *gfx_buf, const u32 width, const u32 height, const bool owns_gfx_buf = false) {
            this->Dispose();
            this->gfx_buf = gfx_buf;
            this->owns_gfx_buf = owns_gfx_buf;
            this->is_sub = is_sub;
            this->x = x;
            this->y = y;
            this->width = width;
            this->height = height;
            NTR_R_SUCCEED();
        }

        inline ntr::Result CreateLoadFrom(const bool is_sub, const int x, const int y, const std::string &path, std::shared_ptr<ntr::fs::FileHandle> file_handle) {
            NTR_R_TRY(this->CreateFrom(is_sub, x, y, path, file_handle));
            NTR_R_TRY(this->Load());
            NTR_R_SUCCEED();
        }

        inline ntr::Result CreateLoadFrom(const bool is_sub, const int x, const int y, ntr::gfx::abgr1555::Color *gfx_buf, const u32 width, const u32 height, const bool owns_gfx_buf = false) {
            NTR_R_TRY(this->CreateFrom(is_sub, x, y, std::move(gfx_buf), width, height, owns_gfx_buf));
            NTR_R_TRY(this->Load());
            NTR_R_SUCCEED();
        }

        inline bool IsLoaded() const {
            return this->sprites != nullptr;
        }

        inline bool IsCreated() const {
            return this->gfx_buf != nullptr;
        }

        inline ntr::Result EnsureLoaded() {
            if(!this->IsLoaded()) {
                NTR_R_TRY(this->Load());
            }

            NTR_R_SUCCEED();
        }

        void Unload();

        void Dispose() {
            this->Unload();
            if(this->IsCreated()) {
                if(this->owns_gfx_buf) {
                    delete[] this->gfx_buf;
                    this->owns_gfx_buf = false;
                }
                this->gfx_buf = nullptr;
            }
        }

        inline constexpr void CenterXInRegion(const int x, const u32 width) {
            this->x = ((width - this->width) / 2) + x;
        }
        
        inline constexpr void CenterYInRegion(const int y, const u32 height) {
            this->y = ((height - this->height) / 2) + y;
        }
        
        inline constexpr void CenterInRegion(const int x, const int y, const u32 width, const u32 height) {
            this->CenterXInRegion(x, width);
            this->CenterYInRegion(y, height);
        }

        inline constexpr void CenterXInScreen() {
            return this->CenterXInRegion(0, ScreenWidth);
        }

        inline constexpr void CenterYInScreen() {
            return this->CenterYInRegion(0, ScreenHeight);
        }
        
        inline constexpr void CenterInScreen() {
            return this->CenterInRegion(0, 0, ScreenWidth, ScreenHeight);
        }

        inline constexpr void CenterXInSprite(const Sprite &other) {
            return this->CenterXInRegion(other.x, other.width);
        }

        inline constexpr void CenterYInSprite(const Sprite &other) {
            return this->CenterYInRegion(other.y, other.height);
        }
        
        inline constexpr void CenterInSprite(const Sprite &other) {
            return this->CenterInRegion(other.x, other.y, other.width, other.height);
        }

        template<typename C>
        inline void SetPixel(const int x, const int y, const C clr) {
            const auto s_x = x / BaseSpriteSizeValue;
            const auto s_y = y / BaseSpriteSizeValue;
            const auto i = s_y * this->sprite_w + s_x;
            this->sprites[i].oam_gfx_ptr[(y % BaseSpriteSizeValue) * BaseSpriteSizeValue + (x % BaseSpriteSizeValue)].raw_val = ntr::gfx::ConvertColor<C, ntr::gfx::abgr1555::Color>(clr).raw_val;
        }
        
        inline constexpr bool IsTouchedBy(const touchPosition pos) {
            return (pos.px > this->x) && (pos.px < (this->x + this->width)) && (pos.py > this->y) && (pos.py < (this->y + this->height));
        }
    };

}