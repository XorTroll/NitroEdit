#include <ui/menu/menu_BMGEditMenu.hpp>
#include <ui/menu/menu_FileBrowseMenu.hpp>
#include <ui/menu/menu_WaveArchiveBrowseMenu.hpp>
#include <ui/menu/menu_ROMEditMenu.hpp>
#include <ui/menu/menu_Keyboard.hpp>
#include <ntr/fmt/fmt_BMG.hpp>

namespace ui::menu {

    namespace {

        ntr::fmt::BMG g_BMG = {};

        std::string FormatEscape(const ntr::fmt::BMG::MessageEscape &esc) {
            std::stringstream strm;
            strm << "{";
            for(ntr::u32 i = 0; i < esc.esc_data.size(); i++) {
                strm << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<ntr::u32>(esc.esc_data.at(i));
                if((i + 1) < esc.esc_data.size()) {
                    strm << "-";
                }
            }
            strm << "}";
            return strm.str();
        }

        ntr::Result BuildMessage(const std::string &input, ntr::fmt::BMG::Message &out_msg) {
            out_msg = {};
            ntr::fmt::BMG::MessageToken cur_token = {};
            std::string cur_escape_byte;

            #define _NEDIT_BMG_MSG_BUILD_CHECK_PUSH_TEXT_TOKEN { \
                if(cur_token.text.length() > 0) { \
                    cur_token.type = ntr::fmt::BMG::MessageTokenType::Text; \
                    out_msg.msg.push_back(cur_token); \
                } \
            }

            #define _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_BYTE { \
                u32 byte; \
                if(ntr::util::ConvertStringToNumber(cur_escape_byte, byte, 16)) { \
                    cur_token.escape.esc_data.push_back(byte & 0xFF); \
                    cur_escape_byte = ""; \
                } \
                else { \
                    return ResultEditBMGInvalidEscapeByte; \
                } \
            }

            for(const auto &ch: input) {
                if(ch == '{') {
                    if(cur_token.type == ntr::fmt::BMG::MessageTokenType::Escape) {
                        return ResultEditBMGUnexpectedEscapeOpen;
                    }

                    _NEDIT_BMG_MSG_BUILD_CHECK_PUSH_TEXT_TOKEN
                    cur_token = {
                        .type = ntr::fmt::BMG::MessageTokenType::Escape
                    };
                }
                else if(ch == '}') {
                    if(cur_token.type != ntr::fmt::BMG::MessageTokenType::Escape) {
                        return ResultEditBMGUnexpectedEscapeClose;
                    }

                    if(!cur_escape_byte.empty()) {
                        _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_BYTE
                    }

                    out_msg.msg.push_back(cur_token);
                    cur_token = {};
                }
                else if(ch == '-') {
                    if(cur_token.type != ntr::fmt::BMG::MessageTokenType::Escape) {
                        return ResultEditBMGUnexpectedEscapeClose;
                    }
                    if(cur_escape_byte.empty()) {
                        return ResultEditBMGInvalidEscapeByte;
                    }

                    _NEDIT_BMG_MSG_BUILD_PUSH_ESCAPE_BYTE
                }
                else {
                    if(cur_token.type == ntr::fmt::BMG::MessageTokenType::Escape) {
                        cur_escape_byte += ch;
                    }
                    else {
                        cur_token.text += ch;
                    }
                }
            }

            if(cur_token.type == ntr::fmt::BMG::MessageTokenType::Escape) {
                return ResultEditBMGUnclosedEscape;
            }

            _NEDIT_BMG_MSG_BUILD_CHECK_PUSH_TEXT_TOKEN

            NTR_R_SUCCEED();
        }

        void OnOtherInput(const u32 k_down) {
            if(k_down & KEY_B) {
                const auto res = ShowSaveConfirmationDialog();
                switch(res) {
                    case DialogResult::Cancel: {
                        return;
                    }
                    case DialogResult::Yes: {
                        SaveNormalFile(g_BMG, "BMG");
                        // Fallthrough
                    }
                    case DialogResult::No: {
                        break;
                    }
                }
                g_BMG = {};
                RestoreFileBrowseMenu();
            }
        }

        void ReloadBMGEditMenu();

        void OnStringSelect(const u32 idx) {
            const KeyboardContext ctx = {
                .only_numeric = false,
                .newline_allowed = true,
                .max_len = 0
            };

            const auto &cur_msg = g_BMG.messages.at(idx);

            std::string edit_msg_str;
            for(const auto &msg_token: cur_msg.msg) {
                switch(msg_token.type) {
                    case ntr::fmt::BMG::MessageTokenType::Escape: {
                        edit_msg_str += FormatEscape(msg_token.escape);
                        break;
                    }
                    case ntr::fmt::BMG::MessageTokenType::Text: {
                        const auto utf8_text = ntr::util::ConvertFromUnicode(msg_token.text);
                        edit_msg_str += utf8_text;
                        break;
                    }
                }
            }

            if(ShowKeyboard(ctx, edit_msg_str)) {
                ntr::fmt::BMG::Message new_msg;
                const auto rc = BuildMessage(edit_msg_str, new_msg);
                if(rc.IsSuccess()) {
                    std::swap(g_BMG.messages.at(idx), new_msg);
                    ReloadBMGEditMenu();
                }
                else {
                    ShowOkDialog("Unable to save message:\n'" + FormatResult(rc) + "'");
                }
            }
        }

        void ReloadBMGEditMenu() {
            std::vector<ScrollMenuEntry> entries;
            auto text_icon_gfx = GetTextIcon();
            for(u32 i = 0; i < g_BMG.messages.size(); i++) {
                entries.push_back({
                    .icon_gfx = text_icon_gfx,
                    .text = "Message " + std::to_string(i),
                    .on_select = std::bind(OnStringSelect, i)
                });
            }
            LoadScrollMenu(entries, OnOtherInput);
        }

    }

    void LoadBMGEditMenu(const std::string &path, std::shared_ptr<ntr::fs::FileHandle> file_handle) {
        ntr::Result rc;
        RunWithDialog("Loading BMG...", [&]() {
            rc = g_BMG.ReadFrom(path, file_handle);
        });

        if(rc.IsSuccess()) {
            ReloadBMGEditMenu();
        }
        else {
            ShowOkDialog("Unable to load BMG:\n'" + FormatResult(rc) + "'");
        }
    }

}