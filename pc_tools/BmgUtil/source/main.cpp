#include <ntr/fmt/fmt_BMG.hpp>
#include <ntr/fs/fs_Stdio.hpp>
#include <ntr/util/util_String.hpp>
#include <args.hxx>

int main(int argc, const char *const *argv) {
    args::ArgumentParser parser("Utility for BMG files");
    args::HelpFlag help(parser, "help", "Displays this help menu", {'h', "help"});
    args::CompletionFlag completion(parser, {"complete"});

    args::Group options(parser, "BMG file options (one required):", args::Group::Validators::Xor);
    args::Flag list(options, "list", "List BMG strings", {'l', "list"});

    args::Group arguments(parser, "Arguments:", args::Group::Validators::All, args::Options::Global);
    args::Positional<std::string> bmg_file(arguments, "bmg_file", "Input BMG file");

    try {
        parser.ParseCLI(argc, argv);
    }
    catch(std::exception &e) {
        std::cerr << parser;
        std::cout << e.what() << std::endl;
        return 1;
    }

    const auto bmg_path = bmg_file.Get();
    ntr::fmt::BMG bmg;
    if(bmg.ReadFrom(bmg_path, std::make_shared<ntr::fs::StdioFileHandle>())) {
        if(list) {
            for(const auto &str: bmg.strings) {
                const auto utf8_str = ntr::util::ConvertFromUnicode(str);
                std::cout << utf8_str << std::endl;
            }
        }
    }
    else {
        std::cerr << "Not a valid BMG file!" << std::endl;
        return 1;
    }

    return 0;
}