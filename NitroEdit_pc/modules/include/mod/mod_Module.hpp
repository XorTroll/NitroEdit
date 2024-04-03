
#pragma once
#include <QString>
#include <QStringList>
#include <QApplication>

namespace nedit::mod {

    struct ModuleVersion {
        quint8 major;
        quint8 minor;
        quint8 micro;
        quint8 bugfix;
    };

    struct ModuleMetadata {
        QString name;
        QString desc;
        QString author;
        ModuleVersion version;
    };

    #define NEDIT_MOD_INITIALIZE_SYMBOL __nedit_mod_Initialize
    using InitializeFunction = bool(*)(ModuleMetadata*);

    #define NEDIT_MOD_PROVIDES_COMMAND_SYMBOL __nedit_mod_ProvidesCommand
    using ProvidesCommandFunction = bool(*)(const QString&);

    #define NEDIT_MOD_HANDLE_COMMAND_SYMBOL __nedit_mod_HandleCommand
    using HandleCommandFunction = void(*)(const QString&, const QStringList&);

    // #define NEDIT_MOD_HANDLE_INPUT_SYMBOL __nedit_mod_HandleInput
    // using HandleInputFunction = void(*)(const QString&);

    #define NEDIT_MOD_DEFINE_START(m_name, m_desc, m_author, ver_major, ver_minor, ver_micro, ver_bugfix) \
        std::vector<QString> g_CommandNames; \
        std::vector<void(*)(const std::vector<std::string>&)> g_CommandHandlers; \
        \
        extern "C" Q_DECL_EXPORT bool NEDIT_MOD_INITIALIZE_SYMBOL(::nedit::mod::ModuleMetadata &out_meta) { \
            out_meta.name = m_name; \
            out_meta.desc = m_desc; \
            out_meta.author = m_author; \
            out_meta.version.major = ver_major; \
            out_meta.version.minor = ver_minor; \
            out_meta.version.micro = ver_micro; \
            out_meta.version.bugfix = ver_bugfix;

    #define NEDIT_MOD_DEFINE_REGISTER_COMMAND(name, handler) \
        g_CommandNames.push_back(name); \
        g_CommandHandlers.push_back(std::addressof(handler));

    #define NEDIT_MOD_DEFINE_END() \
            return true; \
        } \
        \
        extern "C" Q_DECL_EXPORT bool NEDIT_MOD_PROVIDES_COMMAND_SYMBOL(const QString &cmd) { \
            for(const auto &mod_cmd: g_CommandNames) { \
                if(mod_cmd == cmd) { \
                    return true; \
                } \
            } \
            return false; \
        } \
        extern "C" Q_DECL_EXPORT void NEDIT_MOD_HANDLE_COMMAND_SYMBOL(const QString &cmd, const QStringList &args) { \
            for(quint32 i = 0; i < g_CommandNames.size(); i++) { \
                if(cmd == g_CommandNames.at(i)) { \
                    const auto std_args = ::nedit::mod::ConvertQStringListToVector(args); \
                    g_CommandHandlers.at(i)(std_args); \
                    break; \
                } \
            } \
        }

    inline std::vector<std::string> ConvertQStringListToVector(const QStringList &list) {
        const auto q_vec = list.toVector();

        std::vector<std::string> vec;
        vec.reserve(q_vec.size());
        for(const QString &qstr : q_vec) {
            vec.push_back(qstr.toStdString());
        }
        return vec;
    }

}
