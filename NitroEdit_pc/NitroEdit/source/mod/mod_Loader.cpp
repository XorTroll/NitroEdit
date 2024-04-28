#include <mod/mod_Loader.hpp>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>

namespace nedit::mod {

    namespace {

        std::vector<Module> g_Modules;

        #define QUOTE(name) #name
        #define STR(macro) QUOTE(macro)

        #define NEDIT_MOD_TRY_RESOLVE_MOD_SYMBOL(symbol, type, field) { \
            auto symbol_fn = (type)lib->resolve(STR(symbol)); \
            if(symbol_fn == nullptr) { \
                return ResultInvalidModuleSymbols; \
            } \
            out_mod.symbols.field = symbol_fn; \
        }

        ntr::Result TryLoadModule(const QString &path, Module &out_mod) {
            auto lib = std::make_shared<QLibrary>(path);
            if(lib->load()) {
                NEDIT_MOD_TRY_RESOLVE_MOD_SYMBOL(NEDIT_MOD_INITIALIZE_SYMBOL, InitializeFunction, init_fn);
                NEDIT_MOD_TRY_RESOLVE_MOD_SYMBOL(NEDIT_MOD_TRY_HANDLE_COMMAND_SYMBOL, TryHandleCommandFunction, try_handle_cmd_fn);
                NEDIT_MOD_TRY_RESOLVE_MOD_SYMBOL(NEDIT_MOD_TRY_HANDLE_INPUT_SYMBOL, TryHandleInputFunction, try_handle_input_fn);

                if(!out_mod.symbols.init_fn(&out_mod.meta)) {
                    return ResultModuleInitializationFailure;
                }

                RegisterResultDescriptionTable(out_mod.meta.rc_table, out_mod.meta.rc_table_size);

                out_mod.dyn_lib = std::move(lib);
                NTR_R_SUCCEED();
            }
            else {
                return ResultModuleLoadError;
            }
        }

    }

    void LoadModules() {
        const auto cwd = QDir(QCoreApplication::applicationDirPath());
        const auto modules_dir = cwd.filePath(NEDIT_MODULES_DIR);

        QDirIterator iter(modules_dir, QDir::Files);
        while(iter.hasNext()) {
            const auto mod_file = iter.next();

            Module mod;
            if(TryLoadModule(mod_file, mod).IsSuccess()) {
                g_Modules.push_back(std::move(mod));
            }
        }
    }

    std::vector<Module> &GetModules() {
        return g_Modules;
    }

}
