#include <ui/ui_Sprite.hpp>

namespace ui {

    namespace {

        constexpr size_t MaxSpriteCount = 128;

        u64 g_MainSpriteAllocationMaskTop = 0;
        u64 g_MainSpriteAllocationMaskBottom = 0;
        u64 g_SubSpriteAllocationMaskTop = 0;
        u64 g_SubSpriteAllocationMaskBottom = 0;

        constexpr size_t MaskBitCount = sizeof(u64) * CHAR_BIT;
        static_assert(MaxSpriteCount == 2 * MaskBitCount);

        #define BIT64(x) (1ull << static_cast<u64>(x))

        size_t g_MainSize = 0;
        size_t g_SubSize = 0;

        inline bool IsSpriteAllocated(u64 &mask_top, u64 &mask_bottom, const u32 idx) {
            if(idx >= MaskBitCount) {
                return (mask_top >> (idx - MaskBitCount)) & 1;
            }
            else {
                return (mask_bottom >> idx) & 1;
            }
        }

        inline void SetSpriteAllocated(u64 &mask_top, u64 &mask_bottom, const u32 idx) {
            if(idx >= MaskBitCount) {
                mask_top |= BIT64(idx - MaskBitCount);
            }
            else {
                mask_bottom |= BIT64(idx);
            }
        }

        inline void UnsetSpriteAllocated(u64 &mask_top, u64 &mask_bottom, const u32 idx) {
            if(idx >= MaskBitCount) {
                mask_top &= ~BIT64(idx - MaskBitCount);
            }
            else {
                mask_bottom &= ~BIT64(idx);
            }
        }

        // Note: ids are searched from 127 to 0 so that sprites created later are shown above the rest

        bool AllocateSpriteIndexImpl(u64 &mask_top, u64 &mask_bottom, size_t &size, u32 &out_idx) {
            for(u32 i = 0; i < MaxSpriteCount; i++) {
                const auto idx = MaxSpriteCount - i - 1;
                if(!IsSpriteAllocated(mask_top, mask_bottom, idx)) {
                    SetSpriteAllocated(mask_top, mask_bottom, idx);
                    out_idx = idx;
                    size++;
                    return true;
                }
            }
            return false;
        }

        void FreeSpriteIndexImpl(u64 &mask_top, u64 &mask_bottom, size_t &size, const u32 idx) {
            if(idx < MaxSpriteCount) {
                if(IsSpriteAllocated(mask_top, mask_bottom, idx)) {
                    UnsetSpriteAllocated(mask_top, mask_bottom, idx);
                    size--;
                }
            }
        }

        inline bool AllocateSpriteIndex(bool is_sub, u32 &out_idx) {
            return AllocateSpriteIndexImpl(is_sub ? g_SubSpriteAllocationMaskTop : g_MainSpriteAllocationMaskTop, is_sub ? g_SubSpriteAllocationMaskBottom : g_MainSpriteAllocationMaskBottom, is_sub ? g_SubSize : g_MainSize, out_idx);
        }

        inline void FreeSpriteIndex(bool is_sub, u32 idx) {
            return FreeSpriteIndexImpl(is_sub ? g_SubSpriteAllocationMaskTop : g_MainSpriteAllocationMaskTop, is_sub ? g_SubSpriteAllocationMaskBottom : g_MainSpriteAllocationMaskBottom, is_sub ? g_SubSize : g_MainSize, idx);
        }

    }

    ntr::Result Sprite::Load() {
        if(!this->IsCreated()) {
            NTR_R_FAIL(ResultUiSpriteNotCreated);
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
        this->sprites = ntr::util::NewArray<SpriteData>(this->sprite_w * this->sprite_h);
        for(u32 i = 0; i < this->sprite_w * this->sprite_h; i++) {
            if(!AllocateSpriteIndex(this->is_sub, this->sprites[i].idx)) {
                NTR_R_FAIL(ResultUiUnableToAllocateSpriteIndex);
            }
            this->sprites[i].oam_gfx_ptr = reinterpret_cast<ntr::gfx::abgr1555::Color*>(oamAllocateGfx(oam, BaseSpriteSize, SpriteColorFormat_Bmp));
            if(this->sprites[i].oam_gfx_ptr == nullptr) {
                NTR_R_FAIL(ResultUiSpriteInvalidGraphicsData);
            }
        }

        this->Refresh();
        NTR_R_SUCCEED();
    }

    void Sprite::Unload() {
        if(this->sprites != nullptr) {
            auto oam = this->is_sub ? &oamSub : &oamMain;
            for(u32 i = 0; i < this->sprite_w * this->sprite_h; i++) {
                dmaFillHalfWords(0, this->sprites[i].oam_gfx_ptr, BaseSpriteSizeValue * BaseSpriteSizeValue * sizeof(ntr::gfx::abgr1555::Color));
                oamFreeGfx(oam, this->sprites[i].oam_gfx_ptr);
                this->sprites[i].oam_gfx_ptr = nullptr;
                oamSet(oam, this->sprites[i].idx, 0, 0, 0, 15, BaseSpriteSize, SpriteColorFormat_Bmp, nullptr, -1, false, true, false, false, false);
                FreeSpriteIndex(this->is_sub, this->sprites[i].idx);
            }
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
                oamSet(oam, this->sprites[idx].idx, this->x + w * BaseSpriteSizeValue, this->y + h * BaseSpriteSizeValue, 0, 15, BaseSpriteSize, SpriteColorFormat_Bmp, this->sprites[idx].oam_gfx_ptr, -1, false, !this->visible, false, false, false);
            }
        } 
    }

}