
#pragma once
#include <ntr/fmt/fmt_Common.hpp>

namespace ntr::fmt {

    struct SSEQ : public fs::FileFormat {
        
        struct Header : public CommonHeader<0x51455353 /* "SSEQ" */ > {
        };

        struct DataSection : public CommonBlock<0x41544144 /* "DATA" */ > {
            u32 data_offset;
        };

        enum class EventType {
            NoteOn,
            Rest,
            ProgramChange,
            MultiTrack,
            OpenTrack,
            Jump,
            Call,
            /*TODO*/
            Pan,
            Volume,
            MasterVolume,
            Transpose,
            PitchBend,
            PitchBendRange,
            TrackPriority,
            NoteWait,
            Tie,
            PortamentoControl,
            ModulationDepth,
            ModulationSpeed,
            ModulationType,
            ModulationRange,
            Portamento,
            PortamentoTime,
            AttackRate,
            DecayRate,
            SustainRate,
            ReleaseRate,
            LoopStart,
            Expression,
            PrintVariable,
            ModulationDelay,
            Tempo,
            SweepPitch,
            LoopEnd,
            Return,
            EndOfTrack
        };

        union EventOffset {
            u8 offset_bytes[3];
            u32 offset_32;
        };

        struct EventNoteOn {
            u8 velocity;
            u32 duration;
        };

        struct EventRest {
            u32 duration;
        };

        struct EventProgramChange {
            u32 instr_id;
        };

        struct EventMultiTrack {
            u16 track_mask;

            inline bool HasTrack(const u8 id) {
                return (this->track_mask >> id) & 1;
            }
        };

        struct EventOpenTrack {
            u8 track_id;
            EventOffset track_offset;
        };

        struct EventJump {
            EventOffset offset;
        };

        struct EventCall {
            EventOffset offset;
        };

        struct EventPan {
            u8 pan;
        };

        struct EventVolume {
            u8 volume;
        };

        struct EventMasterVolume {
            u8 volume;
        };

        struct EventTranspose {
            u8 transpose;
        };

        struct EventPitchBend {
            i8 bend;
        };

        struct EventPitchBendRange {
            u8 bend_range;
        };

        struct EventTrackPriority {
            u8 priority;
        };

        struct EventNoteWait {
            bool on_off;
        };
        
        struct EventTie {
            bool on_off;
        };

        struct EventPortamentoControl {
            u8 control;
        };

        struct EventModulationDepth {
            bool on_off;
        };

        struct EventModulationSpeed {
            u8 speed;
        };

        struct EventModulationType {
            u8 type;
        };

        struct EventModulationRange {
            u8 range;
        };

        struct EventPortamento {
            bool on_off;
        };

        struct EventPortamentoTime {
            u8 time;
        };

        struct EventAttackRate {
            u8 rate;
        };

        struct EventDecayRate {
            u8 rate;
        };

        struct EventSustainRate {
            u8 rate;
        };

        struct EventReleaseRate {
            u8 rate;
        };

        struct EventLoopStart {
            u8 loop_count;
        };

        struct EventExpression {
            u8 expression;
        };

        struct EventPrintVariable {
            u8 unk_var;
        };

        struct EventModulationDelay {
            u16 delay;
        };

        struct EventTempo {
            u16 tempo;
        };

        struct EventSweepPitch {
            u16 pitch;
        };

        struct EventLoopEnd {
        };

        struct EventReturn {
        };

        struct EventEndOfTrack {
        };

        struct Event {
            EventType type;
            union {
                EventNoteOn note_on;
                EventRest rest;
                EventProgramChange program_change;
                EventMultiTrack multi_track;
                EventOpenTrack open_track;
                EventJump jump;
                EventCall call;
                EventPan pan;
                EventVolume volume;
                EventMasterVolume master_volume;
                EventTranspose transpose;
                EventPitchBend pitch_bend;
                EventPitchBendRange pitch_bend_range;
                EventTrackPriority track_priority;
                EventNoteWait note_wait;
                EventTie tie;
                EventPortamentoControl portamento_control;
                EventModulationDepth modulation_depth;
                EventModulationSpeed modulation_speed;
                EventModulationType modulation_type;
                EventModulationRange modulation_range;
                EventPortamento portamento;
                EventPortamentoTime portamento_time;
                EventAttackRate attack_rate;
                EventDecayRate decay_rate;
                EventSustainRate sustain_rate;
                EventReleaseRate release_rate;
                EventLoopStart loop_start;
                EventExpression expression;
                EventPrintVariable print_variable;
                EventModulationDelay modulation_delay;
                EventTempo tempo;
                EventSweepPitch sweep_pitch;
                EventLoopEnd loop_end;
                EventReturn ret;
                EventEndOfTrack end_of_track;
            } event;
        };

        Header header;
        DataSection data;
        std::vector<SSEQ::Event> events;

        SSEQ() {}
        SSEQ(const SSEQ&) = delete;

        Result ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        Result ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        
        Result WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override {
            NTR_R_FAIL(ResultSSEQWriteNotSupported);
        }
    };

}