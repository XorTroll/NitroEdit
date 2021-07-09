
#pragma once
#include <ntr/ntr_Common.hpp>
#include <fs/fs_Fat.hpp>

namespace ntr {

    struct SDAT : public fs::ExternalFsFileFormat {
        
        struct Header : public CommonHeader<0x54414453 /* "SDAT" */ > {
            u32 symb_offset;
            u32 symb_size;
            u32 info_offset;
            u32 info_size;
            u32 fat_offset;
            u32 fat_size;
            u32 file_offset;
            u32 file_size;
            u8 unk_pad[16];
        };

        struct SymbolRecordEntry {
            u32 name_offset;
            std::string name;
        };

        struct SymbolRecord {
            u32 entry_count;
            std::vector<SymbolRecordEntry> entries;
        };

        struct SequenceArchiveSymbolRecordEntry {
            u32 name_offset;
            std::string name;
            u32 subrecord_offset;
            SymbolRecord subrecord;
        };
        
        struct SequenceArchiveSymbolRecord {
            u32 entry_count;
            std::vector<SequenceArchiveSymbolRecordEntry> entries;
        };

        struct SymbolBlock : public CommonBlock<0x424D5953 /* "SYMB" */ > {
            u32 seq_record_offset;
            u32 seq_arc_record_offset;
            u32 bnk_record_offset;
            u32 wav_arc_record_offset;
            u32 player_record_offset;
            u32 group_record_offset;
            u32 strm_player_record_offset;
            u32 strm_record_offset;
            u8 unk_pad[24];
        };

        struct SequenceInfo {
            u32 file_id;
            u16 bank_id;
            u8 volume;
            u8 channel_prio;
            u8 player_prio;
            u8 player_no;
            u8 unk_pad[2];
        };

        struct SequenceArchiveInfo {
            u32 file_id;
        };

        struct BankInfo {
            u32 file_id;
            u16 wave_arcs[4];
        };

        struct WaveArchiveInfo {
            union {
                struct {
                    u32 file_id : 24;
                    u32 flags : 8;
                };
                u32 val;
            };
        };

        struct PlayerInfo {
            u8 seq_max;
            u8 pad;
            u16 alloc_ch_bit_flag;
            u32 heap_size;
        };

        struct GroupInfo {
            u32 count;
        };

        struct StreamPlayerInfo {
            u8 channel_count;
            u8 channel_nos[16];
            u8 pad[7];
        };

        struct StreamInfo {
            u32 file_id;
            u8 volume;
            u8 player_prio;
            u8 player_no;
            u8 flags;
        };

        template<typename T>
        struct InfoRecordEntry {
            u32 info_offset;
            T info;
        };

        template<typename T>
        struct InfoRecord {
            u32 entry_count;
            std::vector<InfoRecordEntry<T>> entries;
        };

        struct InfoBlock : public CommonBlock<0x4F464E49 /* "INFO" */ > {
            u32 seq_record_offset;
            u32 seq_arc_record_offset;
            u32 bnk_record_offset;
            u32 wav_arc_record_offset;
            u32 player_record_offset;
            u32 group_record_offset;
            u32 strm_player_record_offset;
            u32 strm_record_offset;
            u8 unk_pad[24];
        };

        struct FileAllocationTableRecord {
            u32 offset;
            u32 size;
            u8 unk_pad[8];
        };

        struct FileAllocationTableBlock : public CommonBlock<0x20544146 /* "FAT " */ > {
            u32 record_count;
        };

        struct FileBlock : public CommonBlock<0x454C4946 /* "FILE" */ > {
            u32 file_count;
        };

        Header header;
        SymbolBlock symb;
        InfoBlock info;
        FileAllocationTableBlock fat;
        FileBlock file;

        SymbolRecord seq_symb_record;
        SequenceArchiveSymbolRecord seq_arc_symb_record;
        SymbolRecord bnk_symb_record;
        SymbolRecord wav_arc_symb_record;
        SymbolRecord player_symb_record;
        SymbolRecord group_symb_record;
        SymbolRecord strm_player_symb_record;
        SymbolRecord strm_symb_record;

        InfoRecord<SequenceInfo> seq_info_record;
        InfoRecord<SequenceArchiveInfo> seq_arc_info_record;
        InfoRecord<BankInfo> bnk_info_record;
        InfoRecord<WaveArchiveInfo> wav_arc_info_record;
        InfoRecord<PlayerInfo> player_info_record;
        InfoRecord<GroupInfo> group_info_record;
        InfoRecord<StreamPlayerInfo> strm_player_info_record;
        InfoRecord<StreamInfo> strm_info_record;
        std::vector<FileAllocationTableRecord> fat_records;

        SDAT() {}
        SDAT(const SDAT&) = delete;

        inline bool HasSymbols() {
            return this->header.symb_size > 0;
        }

        bool LocateFile(const std::string &path, u32 &out_file_id);
        bool ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        bool SaveFileSystem() override;
    };

    struct SDATFileHandle : public fs::ExternalFsFileHandle<SDAT> {
        u32 file_id;
        fs::BinaryFile base_bf;

        SDATFileHandle(SDAT &sdat) : fs::ExternalFsFileHandle<SDAT>(sdat) {}

        inline const u32 GetFileOffset() {
            return this->ext_fs_file.fat_records[this->file_id].offset;
        }

        inline const u32 GetFileSize() {
            return this->ext_fs_file.fat_records[this->file_id].size;
        }

        bool ExistsImpl(const std::string &path, size_t &out_size) override;
        bool OpenImpl(const std::string &path) override;
        size_t GetSizeImpl() override;
        bool SetOffsetImpl(const size_t offset, const fs::Position pos) override;
        size_t GetOffsetImpl() override;
        bool ReadImpl(void *read_buf, const size_t read_size) override;
        bool CloseImpl() override;
    };

}