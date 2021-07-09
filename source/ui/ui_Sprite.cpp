#include <ui/ui_Sprite.hpp>

namespace ui {

    namespace {

        constexpr size_t MaxSpriteCount = 128;

        bool g_MainSpriteAllocationTable[MaxSpriteCount] = {};
        bool g_SubSpriteAllocationTable[MaxSpriteCount] = {};
        size_t g_MainSize = 0;
        size_t g_SubSize = 0;

        // Note: ids are searched from 127 to 0 so that sprites created later are shown above the rest

        bool AllocateSpriteIndexImpl(bool *alloc_table, size_t &size, u32 &out_idx) {
            for(u32 i = 0; i < MaxSpriteCount; i++) {
                const auto idx = MaxSpriteCount - i - 1;
                if(!alloc_table[idx]) {
                    alloc_table[idx] = true;
                    out_idx = idx;
                    size++;
                    return true;
                }
            }
            return false;
        }

        void FreeSpriteIndexImpl(bool *alloc_table, size_t &size, u32 idx) {
            if(idx < MaxSpriteCount) {
                if(alloc_table[idx]) {
                    alloc_table[idx] = false;
                    size--;
                }
            }
        }

        inline bool AllocateSpriteIndex(bool is_sub, u32 &out_idx) {
            return AllocateSpriteIndexImpl(is_sub ? g_SubSpriteAllocationTable : g_MainSpriteAllocationTable, is_sub ? g_SubSize : g_MainSize, out_idx);
        }

        inline void FreeSpriteIndex(bool is_sub, u32 idx) {
            return FreeSpriteIndexImpl(is_sub ? g_SubSpriteAllocationTable : g_MainSpriteAllocationTable, is_sub ? g_SubSize : g_MainSize, idx);
        }

    }

    bool Sprite::Load() {
        if(!this->IsCreated()) {
            return false;
        }
        this->Unload();
        this->sprite_w = 0;
        this->sprite_h = 0;
        this->visible = true;
        auto oam = this->is_sub ? &oamSub : &oamMain;
        auto tmp_width = this->width;
        auto tmp_height = this->height;
        while(tmp_width > 0) {
            tmp_width -= std::min(tmp_width, BaseSpriteSizeValue);
            this->sprite_w++;
        }
        while(tmp_height > 0) {
            tmp_height -= std::min(tmp_height, BaseSpriteSizeValue);
            this->sprite_h++;
        }
        this->sprites = util::NewArray<SpriteData>(this->sprite_w * this->sprite_h);
        for(u32 i = 0; i < this->sprite_w * this->sprite_h; i++) {
            if(!AllocateSpriteIndex(this->is_sub, this->sprites[i].idx)) {
                return false;
            }
            this->sprites[i].gfx_ptr = reinterpret_cast<gfx::abgr1555::Color*>(oamAllocateGfx(oam, BaseSpriteSize, SpriteColorFormat_Bmp));
            if(this->sprites[i].gfx_ptr == nullptr) {
                return false;
            }
        }
        this->Refresh();
        return true;
    }

    void Sprite::Unload() {
        if(this->sprites != nullptr) {
            auto oam = this->is_sub ? &oamSub : &oamMain;
            for(u32 i = 0; i < this->sprite_w * this->sprite_h; i++) {
                dmaFillHalfWords(0, this->sprites[i].gfx_ptr, BaseSpriteSizeValue * BaseSpriteSizeValue * sizeof(gfx::abgr1555::Color));
                oamFreeGfx(oam, this->sprites[i].gfx_ptr);
                this->sprites[i].gfx_ptr = nullptr;
                oamSet(oam, this->sprites[i].idx, 0, 0, 0, 15, BaseSpriteSize, SpriteColorFormat_Bmp, nullptr, -1, false, true, false, false, false);
                FreeSpriteIndex(this->is_sub, this->sprites[i].idx);
            }
            delete[] this->sprites;
            this->sprites = nullptr;
            oamUpdate(oam);
        }
    }

    void Sprite::Update() const {
        if(!this->IsCreated()) {
            return;
        }
        if(!this->IsLoaded()) {
            return;
        }
        auto oam = this->is_sub ? &oamSub : &oamMain;
        for(u32 h = 0; h < this->sprite_h; h++) {
            for(u32 w = 0; w < this->sprite_w; w++) {
                const auto idx = h * this->sprite_w + w;
                oamSet(oam, this->sprites[idx].idx, this->x + w * BaseSpriteSizeValue, this->y + h * BaseSpriteSizeValue, 0, 15, BaseSpriteSize, SpriteColorFormat_Bmp, this->sprites[idx].gfx_ptr, -1, false, !this->visible, false, false, false);
            }
        } 
    }

}