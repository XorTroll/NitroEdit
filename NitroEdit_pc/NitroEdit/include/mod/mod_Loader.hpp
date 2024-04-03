
#pragma once
#include <base_Include.hpp>
#include <mod/mod_Module.hpp>
#include <QString>
#include <QLibrary>

namespace nedit::mod {

    struct ModuleSymbols {
        InitializeFunction init_fn;
        ProvidesCommandFunction provides_cmd_fn;
        HandleCommandFunction handle_cmd_fn;
    };

    struct Module {
        ModuleMetadata meta;
        ModuleSymbols symbols;
        std::shared_ptr<QLibrary> dyn_lib;
    };

    void LoadModules();
    std::vector<Module> &GetModules();

}
