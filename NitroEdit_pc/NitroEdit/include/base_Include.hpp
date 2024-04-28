
#pragma once
#include <ntr/ntr_Include.hpp>
#include <QString>

// Define our custom results, aside from the rest defined in libnedit

constexpr ntr::Result ResultModuleLoadError = 0xd001;
constexpr ntr::Result ResultInvalidModuleSymbols = 0xd002;
constexpr ntr::Result ResultModuleInitializationFailure = 0xd003;

constexpr std::pair<ntr::Result, const char*> ResultDescriptionTable[] = {
    { ResultModuleLoadError, "Error loading module" },
    { ResultInvalidModuleSymbols, "Invalid module symbols" },
    { ResultModuleInitializationFailure, "Unable to get module metadata" }
};

QString FormatResult(const ntr::Result rc);

void InitializeResults();
void RegisterResultDescriptionTable(const ntr::ResultDescriptionEntry *table, const size_t table_size);
