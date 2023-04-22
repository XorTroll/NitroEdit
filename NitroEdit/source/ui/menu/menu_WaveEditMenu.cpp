#include <ui/menu/menu_WaveEditMenu.hpp>
#include <ui/menu/menu_WaveBrowseMenu.hpp>
#include <ui/menu/menu_WaveArchiveBrowseMenu.hpp>
#include <ui/menu/menu_SDATEditMenu.hpp>
#include <ui/menu/menu_FileBrowseMenu.hpp>

namespace ui::menu {

    namespace {

        std::vector<ScrollMenuEntry> g_MenuEntries;
        ntr::fmt::SWAR::Sample g_Sample = {};

        constexpr u8 Volume = 60; // TODO: adjustable?

        u8 *g_RecordedSoundData = nullptr;
        u8 *g_RecordedMicrophoneData = nullptr;

        constexpr size_t SecondsToRecord = 5;
        size_t g_RecordedSoundDataSize = 0;
        size_t g_RecordedMicrophoneDataSize = 0;
        size_t g_RecordedSoundDataOffset = 0;

        inline constexpr bool ConvertWaveTypeToSoundFormat(const ntr::fmt::WaveType type, SoundFormat &out_fmt) {
            switch(type) {
                case ntr::fmt::WaveType::PCM8: {
                    out_fmt = SoundFormat_8Bit;
                    return true;
                }
                case ntr::fmt::WaveType::PCM16: {
                    out_fmt = SoundFormat_16Bit;
                    return true;
                }
                case ntr::fmt::WaveType::ADPCM: {
                    out_fmt = SoundFormat_ADPCM;
                    return true;
                }
                default: {
                    return false;
                }
            }
            return false;
        }

        inline constexpr bool ConvertWaveTypeToMicrophoneFormat(const ntr::fmt::WaveType type, MicFormat &out_fmt) {
            switch(type) {
                case ntr::fmt::WaveType::PCM8: {
                    out_fmt = MicFormat_8Bit;
                    return true;
                }
                case ntr::fmt::WaveType::PCM16: {
                    out_fmt = MicFormat_12Bit;
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        void OnRecord(void *data, int len) {
            if(g_RecordedSoundDataOffset > g_RecordedSoundDataSize) {
                return;
            }

            DC_InvalidateRange(data, len);
            const auto copy_len = std::min(static_cast<size_t>(len), g_RecordedSoundDataSize - g_RecordedSoundDataOffset);
            dmaCopy(data, g_RecordedSoundData + g_RecordedSoundDataOffset, copy_len);
            g_RecordedSoundDataOffset += copy_len;
        }

        void RecordImpl(MicFormat mic_fmt) {
            g_RecordedSoundDataSize = g_Sample.info.sample_rate * 2 * SecondsToRecord;
            g_RecordedMicrophoneDataSize = (g_Sample.info.sample_rate * 2) / 30;
            g_RecordedSoundDataOffset = 0;

            g_RecordedSoundData = ntr::util::NewArray<u8>(g_RecordedSoundDataSize);
            g_RecordedMicrophoneData = ntr::util::NewArray<u8>(g_RecordedMicrophoneDataSize);

            soundMicRecord(g_RecordedMicrophoneData, g_RecordedMicrophoneDataSize, mic_fmt, g_Sample.info.sample_rate, OnRecord);

            while(true) {
                scanKeys();
                const auto k_down = keysDown();
                if(k_down & KEY_A) {
                    soundMicOff();
                    break;
                }
            }
        
            g_Sample.info.non_loop_len = g_RecordedSoundDataOffset / sizeof(u32);
        }

        void OnOtherInput(const u32 k_down) {
            if(k_down & KEY_B) {
                RestoreWaveBrowseMenu();
            }
        }

    }

    void InitializeWaveEditMenu() {
        auto music_icon_gfx = GetMusicIcon();
        g_MenuEntries = {
            {
                .icon_gfx = music_icon_gfx,
                .text = "Play sample",
                .on_select = []() {
                    auto &swar = GetCurrentSWAR();
                    auto sample_data = ntr::util::NewArray<u8>(g_Sample.data_size);
                    if(swar.ReadSampleData(g_Sample, sample_data)) {
                        SoundFormat sound_fmt;
                        if(!ConvertWaveTypeToSoundFormat(g_Sample.info.wave_type, sound_fmt)) {
                            ShowOkDialog("Unable to determine sound format...");
                            delete[] sample_data;
                            return;
                        }
                        soundPlaySample(sample_data, sound_fmt, g_Sample.data_size, g_Sample.info.sample_rate, Volume, 64, false, 0);
                    }
                    else {
                        ShowOkDialog("Unable to read sample data...");
                    }
                    delete[] sample_data;
                }
            },
            {
                .icon_gfx = music_icon_gfx,
                .text = "Record sample",
                .on_select = []() {
                    MicFormat mic_fmt;
                    if(!ConvertWaveTypeToMicrophoneFormat(g_Sample.info.wave_type, mic_fmt)) {
                        // Default to 16-bit PCM
                        mic_fmt = MicFormat_12Bit;
						g_Sample.info.wave_type = ntr::fmt::WaveType::PCM16;
                    }
                    SoundFormat sound_fmt;
                    if(!ConvertWaveTypeToSoundFormat(g_Sample.info.wave_type, sound_fmt)) {
                        ShowOkDialog("Unable to determine sound format...");
                        return;
                    }

                    auto stop_record = false;
                    while(!stop_record) {
                        ShowOkDialog("Press [A] to finish and save.\nPress [B] to cancel.\nPress [X] to restart.\n\nClose this to start recording...");
                        ResetButtonTexts();
                        AddButtonText("A", "Finish");
                        AddButtonText("B", "Cancel");
                        AddButtonText("X", "Restart");
                        RunWithDialog("Recording...", [&]() {
                            RecordImpl(mic_fmt);
                        });

                        soundPlaySample(g_RecordedSoundData, sound_fmt, g_RecordedSoundDataOffset, g_Sample.info.sample_rate, Volume, 64, false, 0);

                        const auto res = ShowSaveConfirmationDialog();
                        switch(res) {
                            case DialogResult::Yes: {
                                auto &sdat = GetCurrentSDAT();
                                auto &swar = GetCurrentSWAR();
                                auto ok = false;
                                RunWithDialog("Saving recorded sample...", [&]() {
                                    ok = swar.WriteSampleData(g_Sample, g_RecordedSoundData, g_RecordedSoundDataOffset, std::make_shared<ntr::fmt::SDATFileHandle>(sdat));
                                });
                                if(ok) {
                                    ShowOkDialog("The sample was\nsuccessfully saved!");
                                    g_Sample.data_size = g_RecordedSoundDataOffset;
                                }
                                else {
                                    ShowOkDialog("Unable to save sample...");
                                }
                                // Fallthrough
                            }
                            case DialogResult::No: {
                                stop_record = true;
                                // Fallthrough
                            }
                            default: {
                                break;
                            }
                        }
                        delete[] g_RecordedSoundData;
                        delete[] g_RecordedMicrophoneData;
                    }
                }
            }
        };
    }

    void LoadWaveEditMenu(const u32 idx) {
        auto &swar = GetCurrentSWAR();
        g_Sample = swar.samples[idx];
        LoadScrollMenu(g_MenuEntries, OnOtherInput);
    }

}