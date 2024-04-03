#include <ntr/fmt/fmt_SSEQ.hpp>

namespace ntr::fmt {

    Result SSEQ::ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.Read(this->header));
        if(!this->header.IsValid()) {
            NTR_R_FAIL(ResultSSEQInvalidHeader);
        }
        
        NTR_R_TRY(bf.Read(this->data));
        if(!this->data.IsValid()) {
            NTR_R_FAIL(ResultSSEQInvalidDataSection);
        }

        NTR_R_SUCCEED();
    }

    Result SSEQ::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.SetAbsoluteOffset(this->data.data_offset));

        // TODO: finish event support

        this->events.clear();
        /*
        while(true) {
            u8 type;
            if(!bf.Read(type)) {
                break;
            }

            switch(type) {
                case 0x00 ... 0x7F: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::NoteOn,
                        .event = {
                            .note_on = {
                                .velocity = type
                            }
                        }
                    };

                    if(!bf.ReadLEB128(evt.event.note_on.duration)) {
                        return false;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0x80: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::Rest,
                        .event = {
                            .rest = {}
                        }
                    };

                    if(!bf.ReadLEB128(evt.event.rest.duration)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0x81: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::ProgramChange,
                        .event = {
                            .program_change = {}
                        }
                    };

                    if(!bf.ReadLEB128(evt.event.program_change.instr_id)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xFE: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::MultiTrack,
                        .event = {
                            .multi_track = {}
                        }
                    };

                    if(!bf.Read(evt.event.multi_track.track_mask)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0x93: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::OpenTrack,
                        .event = {
                            .open_track = {}
                        }
                    };

                    if(!bf.Read(evt.event.open_track.track_id)) {
                        break;
                    }
                    if(!bf.Read(evt.event.open_track.track_offset.offset_bytes)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0x94: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::Jump,
                        .event = {
                            .jump = {}
                        }
                    };

                    if(!bf.Read(evt.event.jump.offset.offset_bytes)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0x95: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::Call,
                        .event = {
                            .jump = {}
                        }
                    };

                    if(!bf.Read(evt.event.call.offset.offset_bytes)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                // ...
                case 0xC0: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::Pan,
                        .event = {
                            .pan = {}
                        }
                    };

                    if(!bf.Read(evt.event.pan.pan)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xC1: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::Volume,
                        .event = {
                            .volume = {}
                        }
                    };

                    if(!bf.Read(evt.event.volume.volume)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xC2: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::MasterVolume,
                        .event = {
                            .master_volume = {}
                        }
                    };

                    if(!bf.Read(evt.event.master_volume.volume)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xC3: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::Transpose,
                        .event = {
                            .transpose = {}
                        }
                    };

                    if(!bf.Read(evt.event.transpose.transpose)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xC4: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::PitchBend,
                        .event = {
                            .pitch_bend = {}
                        }
                    };

                    if(!bf.Read(evt.event.pitch_bend.bend)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xC5: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::PitchBendRange,
                        .event = {
                            .pitch_bend_range = {}
                        }
                    };

                    if(!bf.Read(evt.event.pitch_bend_range.bend_range)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xC6: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::TrackPriority,
                        .event = {
                            .track_priority = {}
                        }
                    };

                    if(!bf.Read(evt.event.track_priority.priority)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xC7: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::NoteWait,
                        .event = {
                            .note_wait = {}
                        }
                    };

                    if(!bf.Read(evt.event.note_wait.on_off)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xC8: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::Tie,
                        .event = {
                            .tie = {}
                        }
                    };

                    if(!bf.Read(evt.event.tie.on_off)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xC9: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::PortamentoControl,
                        .event = {
                            .portamento_control = {}
                        }
                    };

                    if(!bf.Read(evt.event.portamento_control.control)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xCA: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::ModulationDepth,
                        .event = {
                            .modulation_depth = {}
                        }
                    };

                    if(!bf.Read(evt.event.modulation_depth.on_off)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xCB: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::ModulationSpeed,
                        .event = {
                            .modulation_speed = {}
                        }
                    };

                    if(!bf.Read(evt.event.modulation_speed.speed)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xCC: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::ModulationType,
                        .event = {
                            .modulation_type = {}
                        }
                    };

                    if(!bf.Read(evt.event.modulation_type.type)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xCD: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::ModulationRange,
                        .event = {
                            .modulation_range = {}
                        }
                    };

                    if(!bf.Read(evt.event.modulation_range.range)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xCE: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::Portamento,
                        .event = {
                            .portamento = {}
                        }
                    };

                    if(!bf.Read(evt.event.portamento.on_off)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xCF: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::PortamentoTime,
                        .event = {
                            .portamento_time = {}
                        }
                    };

                    if(!bf.Read(evt.event.portamento_time.time)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xD0: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::AttackRate,
                        .event = {
                            .attack_rate = {}
                        }
                    };

                    if(!bf.Read(evt.event.attack_rate.rate)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xD1: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::DecayRate,
                        .event = {
                            .decay_rate = {}
                        }
                    };

                    if(!bf.Read(evt.event.decay_rate.rate)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xD2: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::SustainRate,
                        .event = {
                            .sustain_rate = {}
                        }
                    };

                    if(!bf.Read(evt.event.sustain_rate.rate)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xD3: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::ReleaseRate,
                        .event = {
                            .release_rate = {}
                        }
                    };

                    if(!bf.Read(evt.event.release_rate.rate)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xD4: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::LoopStart,
                        .event = {
                            .master_volume = {}
                        }
                    };

                    if(!bf.Read(evt.event.loop_start.loop_count)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xD5: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::Expression,
                        .event = {
                            .expression = {}
                        }
                    };

                    if(!bf.Read(evt.event.expression.expression)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xD6: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::PrintVariable,
                        .event = {
                            .print_variable = {}
                        }
                    };

                    if(!bf.Read(evt.event.print_variable.unk_var)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xE0: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::ModulationDelay,
                        .event = {
                            .modulation_delay = {}
                        }
                    };

                    if(!bf.Read(evt.event.modulation_delay.delay)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xE1: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::Tempo,
                        .event = {
                            .tempo = {}
                        }
                    };

                    if(!bf.Read(evt.event.tempo.tempo)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xE3: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::SweepPitch,
                        .event = {
                            .sweep_pitch = {}
                        }
                    };

                    if(!bf.Read(evt.event.sweep_pitch.pitch)) {
                        break;
                    }

                    this->events.push_back(evt);
                    break;
                }
                case 0xFC: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::LoopEnd,
                        .event = {
                            .loop_end = {}
                        }
                    };

                    this->events.push_back(evt);
                    break;
                }
                case 0xFD: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::Return,
                        .event = {
                            .ret = {}
                        }
                    };

                    this->events.push_back(evt);
                    break;
                }
                case 0xFF: {
                    SSEQ::Event evt = {
                        .type = SSEQ::EventType::EndOfTrack,
                        .event = {
                            .end_of_track = {}
                        }
                    };

                    this->events.push_back(evt);
                    break;
                }
                default: {
                    printf("Unknown event type 0x%X!\n", type);
                    break;
                }
            }
        }
        */

        NTR_R_SUCCEED();
    }

}