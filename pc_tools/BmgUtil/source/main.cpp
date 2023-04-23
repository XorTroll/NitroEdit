#include <ntr/fmt/fmt_BMG.hpp>
#include <ntr/fs/fs_Stdio.hpp>
#include <ntr/util/util_String.hpp>
#include <args.hxx>
#include <iomanip>
#include <fstream>

namespace {

    std::string FormatEncoding(const ntr::fmt::BMG::Encoding enc) {
        switch(enc) {
            case ntr::fmt::BMG::Encoding::CP1252: {
                return "CP1252";
            }
            case ntr::fmt::BMG::Encoding::UTF16: {
                return "UTF-16";
            }
            case ntr::fmt::BMG::Encoding::UTF8: {
                return "UTF-8";
            }
            case ntr::fmt::BMG::Encoding::ShiftJIS: {
                return "Shift-JIS";
            }
        }
        return "<unk>";
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

}

int main(int argc, char **argv) {
    args::ArgumentParser parser("Utility for BMG files");
    args::HelpFlag help(parser, "help", "Displays this help menu", {'h', "help"});

    args::Group commands(parser, "Commands:", args::Group::Validators::Xor);

    args::Command list(commands, "list", "List BMG file strings");
    args::Group list_required(list, "", args::Group::Validators::All);
    args::ValueFlag<std::string> list_bmg_file(list_required, "bmg_file", "Input BMG file", {'i', "in"});
    args::Flag list_verbose(list, "verbose", "Print more information, not just the strings", {'v', "verbose"});

    args::Command create(commands, "create", "Create BMG file(s) from text file");
    args::Group create_required(create, "", args::Group::Validators::All);
    args::ValueFlag<std::string> create_txt_file(create_required, "txt_file", "Input text file", {'i', "in"});
    args::ValueFlag<std::string> create_out_bmg_file(create_required, "out_bmg_file", "Output BMG file", {'o', "out"});
    args::ValueFlag<std::string> create_encoding(create_required, "encoding", "Text encoding ('utf8' or 'utf16' are supported)", {"enc"});

    try {
        parser.ParseCLI(argc, argv);
    }
    catch(std::exception &e) {
        std::cerr << parser;
        std::cout << e.what() << std::endl;
        return 1;
    }

    if(list) {
        const auto bmg_path = list_bmg_file.Get();
        const auto verbose = list_verbose.Get();

        ntr::fmt::BMG bmg;
        if(bmg.ReadFrom(bmg_path, std::make_shared<ntr::fs::StdioFileHandle>())) {
            if(verbose) {
                std::cout << " - Encoding: " << FormatEncoding(bmg.header.encoding) << std::endl;
                std::cout << " - String extra attribute size: 0x" << std::hex << bmg.info.GetAttributesSize() << std::dec << std::endl;

                std::cout << " - Strings:" << std::endl;
            }
            for(const auto &str: bmg.strings) {
                const auto utf8_str = ntr::util::ConvertFromUnicode(str.str);
                std::cout << utf8_str;

                if(verbose) {
                    if(bmg.info.GetAttributesSize() > 0) {
                        std::cout << " (";
                        for(const auto &attr_byte: str.attrs) {
                            std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<ntr::u32>(attr_byte);
                        }
                        std::cout << ")";
                    }
                }

                std::cout << std::endl;
            }
        }
        else {
            std::cerr << "'" << bmg_path << "' is not a valid BMG file!" << std::endl;
            return 1;
        }
    }
    else if(create) {
        std::vector<ntr::fmt::BMG::String> strs;

        const auto enc_str = create_encoding.Get();
        ntr::fmt::BMG::Encoding enc;
        if(!ReadEncoding(enc_str, enc)) {
            std::cerr << "'" << enc_str << "' is not a supported encoding!" << std::endl;
            return 1;
        }

        const auto txt_path = create_txt_file.Get();
        std::ifstream ifs(txt_path);
        if(!ifs) {
            std::cerr << "'" << txt_path << "' is not a valid text file!" << std::endl;
            return 1;
        }

        std::string txt_str;
        while(std::getline(ifs, txt_str)) {
            const ntr::fmt::BMG::String str = {
                .str = ntr::util::ConvertToUnicode(txt_str),
                .attrs = {}
            };
            strs.push_back(str);
        }

        ntr::fmt::BMG bmg;
        if(!bmg.CreateFrom(enc, 0, strs, 0, ntr::fs::FileCompression::None)) {
            std::cerr << "Unable to create BMG file!" << std::endl;
            return 1;
        }

        std::cout << "saving..." << std::endl;

        const auto out_bmg_path = create_out_bmg_file.Get();
        if(!bmg.WriteTo(out_bmg_path, std::make_shared<ntr::fs::StdioFileHandle>()))  {
            std::cerr << "Unable to save BMG file!" << std::endl;
            return 1;
        }
    }

    return 0;
}