#include <mod/mod_Module.hpp>
#include <args.hxx>
#include <ntr/fmt/fmt_BMG.hpp>
#include <ntr/fs/fs_Stdio.hpp>
#include <ntr/util/util_String.hpp>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ui/ui_BmgSubWindow.hpp>
#include <QFileInfo>

namespace {

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

    bool BuildMessage(const std::string &input, ntr::fmt::BMG::Message &out_msg) {
        out_msg = {};
        ntr::fmt::BMG::MessageToken cur_token = {};
        std::string cur_escape_byte;

        #define _NEDIT_MOD_BMG_MSG_BUILD_CHECK_PUSH_TEXT_TOKEN \
            if(cur_token.text.length() > 0) { \
                cur_token.type = ntr::fmt::BMG::MessageTokenType::Text; \
                out_msg.msg.push_back(cur_token); \
            }

        #define _NEDIT_MOD_BMG_MSG_BUILD_PUSH_ESCAPE_BYTE \
            try { \
                int byte = std::stoi(cur_escape_byte, nullptr, 16); \
                cur_token.escape.esc_data.push_back(byte & 0xFF); \
                cur_escape_byte = ""; \
            } \
            catch(std::exception&) { \
                std::cerr << "Formatting error: invalid escape byte supplied (" << cur_escape_byte << "), they must be in hexadecimal like here: {FF-00-AA-12}" << std::endl; \
                return false; \
            }

        for(const auto &ch: input) {
            if(ch == '{') {
                if(cur_token.type == ntr::fmt::BMG::MessageTokenType::Escape) {
                    std::cerr << "Formatting error: parsed escape opening symbol '{' with an already opened escape" << std::endl;
                    return false;
                }

                _NEDIT_MOD_BMG_MSG_BUILD_CHECK_PUSH_TEXT_TOKEN
                cur_token = {
                    .type = ntr::fmt::BMG::MessageTokenType::Escape
                };
            }
            else if(ch == '}') {
                if(cur_token.type != ntr::fmt::BMG::MessageTokenType::Escape) {
                    std::cerr << "Formatting error: parsed escape closing symbol '}' without any previous escape opening symbol '{'" << std::endl;
                    return false;
                }

                if(!cur_escape_byte.empty()) {
                    _NEDIT_MOD_BMG_MSG_BUILD_PUSH_ESCAPE_BYTE
                }

                out_msg.msg.push_back(cur_token);
                cur_token = {};
            }
            else if(ch == '-') {
                if(cur_token.type != ntr::fmt::BMG::MessageTokenType::Escape) {
                    std::cerr << "Formatting error: parsed escape closing symbol '}' without any previous escape opening symbol '{'" << std::endl;
                    return false;
                }
                if(cur_escape_byte.empty()) {
                    std::cerr << "Formatting error: found empty escape byte" << std::endl;
                    return false;
                }

                _NEDIT_MOD_BMG_MSG_BUILD_PUSH_ESCAPE_BYTE
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
            std::cerr << "Formatting error: reached end with unclosed escape" << std::endl;
            return false;
        }

        _NEDIT_MOD_BMG_MSG_BUILD_CHECK_PUSH_TEXT_TOKEN

        return true;
    }

    void PrintMessage(const ntr::fmt::BMG::Message &msg) {
        for(const auto &msg_token: msg.msg) {
            switch(msg_token.type) {
                case ntr::fmt::BMG::MessageTokenType::Escape: {
                    std::cout << FormatEscape(msg_token.escape);
                    break;
                }
                case ntr::fmt::BMG::MessageTokenType::Text: {
                    const auto utf8_text = ntr::util::ConvertFromUnicode(msg_token.text);
                    std::cout << utf8_text;
                    break;
                }
            }
        }
    }

    bool ReadEncoding(const std::string &enc_str, ntr::fmt::BMG::Encoding &out_enc) {
        const auto enc_l_str = ntr::util::ToLowerString(enc_str);
        
        if((enc_l_str == "utf16") || (enc_l_str == "utf-16")) {
            out_enc = ntr::fmt::BMG::Encoding::UTF16;
            return true;
        }
        if((enc_l_str == "utf8") || (enc_l_str == "utf-8")) {
            out_enc = ntr::fmt::BMG::Encoding::UTF8;
            return true;
        }

        return false;
    }

    void ListBmg(const std::string &bmg_path, const bool verbose) {
        ntr::fmt::BMG bmg;
        const auto rc = bmg.ReadFrom(bmg_path, std::make_shared<ntr::fs::StdioFileHandle>());
        if(rc.IsSuccess()) {
            if(verbose) {
                std::cout << " - Encoding: " << FormatEncoding(bmg.header.encoding).toStdString() << std::endl;
                std::cout << " - Message extra attribute size: 0x" << std::hex << bmg.info.GetAttributesSize() << std::dec << std::endl;

                std::cout << " - Messages:" << std::endl;
            }
            for(const auto &msg: bmg.messages) {
                PrintMessage(msg);

                if(verbose) {
                    if(bmg.info.GetAttributesSize() > 0) {
                        std::cout << " (";
                        for(const auto &attr_byte: msg.attrs) {
                            std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<ntr::u32>(attr_byte);
                        }
                        std::cout << ")";
                        
                    }
                }

                std::cout << std::endl;
            }
        }
        else {
            std::cerr << "Unable to read BMG file '" << bmg_path << "': " << rc.GetDescription() << std::endl;
            return;
        }
    }

    void GetBmgString(const std::string &bmg_path, const std::string &idx_str) {
        int idx;
        try {
            idx = std::stoi(idx_str);
        }
        catch(std::invalid_argument &ex) {
            std::cerr << "Invalid index supplied (invalid argument): " << ex.what() << std::endl;
            return;
        }
        catch(std::out_of_range &ex) {
            std::cerr << "Invalid index supplied (out of range): " << ex.what() << std::endl;
            return;
        }

        ntr::fmt::BMG bmg;
        const auto rc = bmg.ReadFrom(bmg_path, std::make_shared<ntr::fs::StdioFileHandle>());
        if(rc.IsSuccess()) {
            if(idx < bmg.messages.size()) {
                const auto &msg = bmg.messages.at(idx);
                PrintMessage(msg);
                std::cout << std::endl;
            }
            else {
                std::cerr << "Invalid index supplied (out of bounds): BMG only has " << bmg.messages.size() << " messages..." << std::endl;
                return;
            }
        }
        else {
            std::cerr << "Unable to read BMG file '" << bmg_path << "': " << rc.GetDescription() << std::endl;
            return;
        }
    }

    void CreateBmg(const std::string &enc_str, const std::string &txt_path, const std::string &out_bmg_path) {
        ntr::fmt::BMG::Encoding enc;
        if(!ReadEncoding(enc_str, enc)) {
            std::cerr << "'" << enc_str << "' is not a supported encoding!" << std::endl;
            return;
        }
        std::ifstream ifs(txt_path);
        if(!ifs) {
            std::cerr << "'" << txt_path << "' is not a valid text file!" << std::endl;
            return;
        }

        std::vector<ntr::fmt::BMG::Message> msgs;
        std::string txt_str;
        while(std::getline(ifs, txt_str)) {
            ntr::fmt::BMG::Message msg;

            if(!BuildMessage(txt_str, msg)) {
                return;
            }

            msgs.push_back(msg);
        }

        ntr::fmt::BMG bmg;
        auto rc = bmg.CreateFrom(enc, false, 0, msgs, 0);
        if(rc.IsFailure()) {
            std::cerr << "Unable to create BMG file: " << rc.GetDescription() << std::endl;
            return;
        }

        std::cout << "saving..." << std::endl;

        rc = bmg.WriteTo(out_bmg_path, std::make_shared<ntr::fs::StdioFileHandle>());
        if(rc.IsFailure())  {
            std::cerr << "Unable to save BMG file: " << rc.GetDescription() << std::endl;
            return;
        }
    }

    void HandleCommand(const std::vector<std::string> &args) {
        args::ArgumentParser parser("Module for BMG files");
        args::HelpFlag help(parser, "help", "Displays this help menu", {'h', "help"});

        args::Group commands(parser, "Commands:", args::Group::Validators::Xor);

        args::Command list(commands, "list", "List BMG message strings");
        args::Group list_required(list, "", args::Group::Validators::All);
        args::ValueFlag<std::string> list_bmg_file(list_required, "bmg_file", "Input BMG file", {'i', "in"});
        args::Flag list_verbose(list, "verbose", "Print more information, not just the message strings", {'v', "verbose"});

        args::Command get(commands, "get", "Get BMG message by index");
        args::Group get_required(get, "", args::Group::Validators::All);
        args::ValueFlag<std::string> get_bmg_file(get_required, "bmg_file", "Input BMG file", {'i', "in"});
        args::ValueFlag<std::string> get_idx(get_required, "idx", "BMG message index", {"idx"});

        args::Command create(commands, "create", "Create BMG file(s) from text file");
        args::Group create_required(create, "", args::Group::Validators::All);
        args::ValueFlag<std::string> create_txt_file(create_required, "txt_file", "Input text file", {'i', "in"});
        args::ValueFlag<std::string> create_out_bmg_file(create_required, "out_bmg_file", "Output BMG file", {'o', "out"});
        args::ValueFlag<std::string> create_encoding(create_required, "encoding", "Message encoding ('utf8' or 'utf16' are supported)", {"enc"});

        try {
            parser.ParseArgs(args);
        }
        catch(std::exception &e) {
            std::cerr << parser;
            std::cout << e.what() << std::endl;
            return;
        }

        if(list) {
            const auto bmg_path = list_bmg_file.Get();
            const auto verbose = list_verbose.Get();
            ListBmg(bmg_path, verbose);
        }
        else if(get) {
            const auto bmg_path = get_bmg_file.Get();
            const auto idx_str = get_idx.Get();
            GetBmgString(bmg_path, idx_str);
        }
        else if(create) {
            const auto enc_str = create_encoding.Get();
            const auto txt_path = create_txt_file.Get();
            const auto out_bmg_path = create_out_bmg_file.Get();
            CreateBmg(enc_str, txt_path, out_bmg_path);
        }
    }

}

NEDIT_MOD_DEFINE_START(
    "Bmg",
    "BMG support and utilities",
    "XorTroll",
    NEDIT_MAJOR,NEDIT_MINOR,NEDIT_MICRO,NEDIT_BUGFIX,
    ResultDescriptionTable, std::size(ResultDescriptionTable)
)

NEDIT_MOD_DEFINE_REGISTER_COMMAND("bmg", HandleCommand)

NEDIT_MOD_DEFINE_END()

NEDIT_MOD_SYMBOL bool NEDIT_MOD_TRY_HANDLE_INPUT_SYMBOL(const QString &path, std::shared_ptr<ntr::fs::FileHandle> file_handle, nedit::mod::Context *ctx) {
    auto bmg = std::make_shared<ntr::fmt::BMG>();
    const auto rc = bmg->ReadFrom(path.toStdString(), file_handle);
    if(rc.IsSuccess()) {
        auto subwin = new ui::BmgSubWindow(ctx, bmg, file_handle);

        QFileInfo file_info(path);
        subwin->setWindowTitle("BMG editor - " + file_info.fileName());

        ctx->ShowSubWindow(subwin);
        return true;
    }
    else {
        return false;
    }
}
