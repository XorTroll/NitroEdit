
#pragma once
#include <ntr/fmt/nfs/nfs_NitroFs.hpp>
#include <ntr/gfx/gfx_BannerIcon.hpp>
#include <ntr/util/util_String.hpp>
#include <ntr/util/util_System.hpp>

namespace ntr::fmt {

    struct ROM : public nfs::NitroFsFileFormat {

        // TODO: use this
        enum class NDSRegion : u8 {
            Normal = 0x00,
            Korea = 0x40,
            China = 0x80
        };

        enum class UnitCode : u8 {
            NDS = 0x00,
            NDS_NDSi = 0x02,
            NDSi = 0x03
        };

        struct Header {
            char game_title[12];
            char game_code[4];
            char maker_code[2];
            UnitCode unit_code;
            u8 encryption_seed_select;
            u8 device_capacity;
            u8 reserved_1[7];
            union {
                u16 ndsi_game_revision;
                struct {
                    u8 reserved;
                    u8 region;
                } nds;
            } game_revision;
            u8 version;
            u8 flags;
            u32 arm9_rom_offset;
            u32 arm9_entry_address;
            u32 arm9_ram_address;
            u32 arm9_size;
            u32 arm7_rom_offset;
            u32 arm7_entry_address;
            u32 arm7_ram_address;
            u32 arm7_size;
            u32 fnt_offset;
            u32 fnt_size;
            u32 fat_offset;
            u32 fat_size;
            u32 arm9_ovl_offset;
            u32 arm9_ovl_size;
            u32 arm7_ovl_offset;
            u32 arm7_ovl_size;
            u32 normal_card_control_register_settings;
            u32 secure_card_control_register_settings;
            u32 banner_offset;
            u16 secure_area_crc;
            u16 secure_transfer_timeout;
            u32 arm9_autoload;
            u32 arm7_autoload;
            u64 secure_disable;
            u32 ntr_region_size;
            u32 header_size;
            u8 reserved_2[56];
            u8 nintendo_logo[156];
            u16 nintendo_logo_crc;
            u16 header_crc;
            u8 reserved_debugger[32];

            // Note: helpers since these strings don't neccessarily end with a null character, so std::string(<c_str>) wouldn't work as expected there

            inline std::string GetGameTitle() {
                return util::GetNonNullTerminatedCString(this->game_title);
            }

            inline void SetGameTitle(const std::string &game_title_str) {
                return util::SetNonNullTerminatedCString(this->game_title, game_title_str);
            }

            inline std::string GetGameCode() {
                return util::GetNonNullTerminatedCString(this->game_code);
            }

            inline void SetGameCode(const std::string &game_code_str) {
                return util::SetNonNullTerminatedCString(this->game_code, game_code_str);
            }
            
            inline std::string GetMakerCode() {
                return util::GetNonNullTerminatedCString(this->maker_code);
            }

            inline void SetMakerCode(const std::string &maker_code_str) {
                return util::SetNonNullTerminatedCString(this->maker_code, maker_code_str);
            }
        };
        static_assert(sizeof(Header) == 0x180);

        static constexpr u32 GameTitleLength = 128;

        struct Banner {
            u8 version;
            u8 reserved_1;
            u16 crc16_v1;
            u8 reserved_2[28];
            u8 icon_chr[gfx::IconCharSize];
            u8 icon_plt[gfx::IconPaletteSize];
            char16_t game_titles[static_cast<u32>(util::SystemLanguage::Count)][GameTitleLength];

            inline std::u16string GetGameTitle(const util::SystemLanguage lang) {
                if(lang < util::SystemLanguage::Count) {
                    return util::GetNonNullTerminatedCString(this->game_titles[static_cast<u32>(lang)]);
                }
                return u"";
            }

            inline void SetGameTitle(const util::SystemLanguage lang, const std::u16string &game_title_str) {
                if(lang < util::SystemLanguage::Count) {
                    util::SetNonNullTerminatedCString(this->game_titles[static_cast<u32>(lang)], game_title_str);
                }
            }
        };

        Header header;
        Banner banner;

        ROM() {}
        ROM(const ROM&) = delete;

        bool GetAlignmentBetweenFileData(size_t &out_align) override {
            out_align = 0x200;
            return true;
        }

        size_t GetFatEntriesOffset() override {
            return this->header.fat_offset;
        }

        size_t GetFatEntryCount() override {
            return this->header.fat_size / sizeof(nfs::FileAllocationTableEntry);
        }

        Result OnFileSystemWrite(fs::BinaryFile &w_bf, const ssize_t size_diff) override {
            size_t actual_rom_size;
            NTR_R_TRY(w_bf.GetAbsoluteOffset(actual_rom_size));
            this->header.ntr_region_size += size_diff;
            auto capacity_size = actual_rom_size;
            capacity_size |= capacity_size >> 16;
            capacity_size |= capacity_size >> 8;
            capacity_size |= capacity_size >> 4;
            capacity_size |= capacity_size >> 2;
            capacity_size |= capacity_size >> 1;
            capacity_size++;
            if(capacity_size <= 0x20000) {
                capacity_size = 0x20000;
            }
            auto capacity = -18;
            while(capacity_size != 0) {
                capacity_size >>= 1;
                capacity++;
            }
            this->header.device_capacity = (capacity < 0) ? 0 : static_cast<u8>(capacity);

            NTR_R_TRY(w_bf.SetAbsoluteOffset(0));
            NTR_R_TRY(w_bf.Write(this->header));

            NTR_R_TRY(w_bf.SetAbsoluteOffset(this->header.banner_offset));
            NTR_R_TRY(w_bf.Write(this->banner));

            NTR_R_SUCCEED();
        }
        
        Result ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        Result ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
    };

    using ROMFileHandle = nfs::NitroFsFileHandle<ROM>;

}