
#pragma once
#include <base_Include.hpp>
#include <mod/mod_Module.hpp>
#include <QString>
#include <QLibrary>
#include <QMdiSubWindow>

namespace nedit::mod {

    struct ModuleSymbols {
        InitializeFunction init_fn;
        TryHandleCommandFunction try_handle_cmd_fn;
        TryHandleInputFunction try_handle_input_fn;
    };

    struct Module {
        ModuleMetadata meta;
        ModuleSymbols symbols;
        std::shared_ptr<QLibrary> dyn_lib;
    };

    void LoadModules();
    std::vector<Module> &GetModules();

}
