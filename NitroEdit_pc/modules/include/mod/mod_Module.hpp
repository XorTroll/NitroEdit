
#pragma once
#include <ntr/fs/fs_BinaryFile.hpp>
#include <QString>
#include <QStringList>
#include <QApplication>
#include <QEvent>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QStatusBar>
#include <QCloseEvent>
#include <QMainWindow>

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
        const ntr::ResultDescriptionEntry *rc_table;
        size_t rc_table_size;
    };

    struct Context {
        using OnSubWindowClosedCallback = void(*)(QMainWindow*);
        using OnFocusedSubWindowSaveCallback = bool(*)(QMainWindow*);

        QMdiArea *sub_window_area;
        QMainWindow *main_win;
        OnSubWindowClosedCallback on_sub_window_closed;
        OnFocusedSubWindowSaveCallback on_focused_sub_window_save;

        inline Context(QMdiArea *sub_window_area, QMainWindow *main_win, OnSubWindowClosedCallback on_sub_window_closed, OnFocusedSubWindowSaveCallback on_focused_sub_window_save) : sub_window_area(sub_window_area), main_win(main_win), on_sub_window_closed(on_sub_window_closed), on_focused_sub_window_save(on_focused_sub_window_save) {}

        inline void ShowSubWindow(QMdiSubWindow *window)  {
            this->sub_window_area->addSubWindow(window);
            window->show();
        }

        inline void NotifySubWindowClosed() {
            this->on_sub_window_closed(this->main_win);
        }

        inline bool SaveFocusedSubWindow() {
            return this->on_focused_sub_window_save(this->main_win);
        }
    };

    class SubWindow : public QMdiSubWindow {
        public:
            inline SubWindow(Context *ctx) : QMdiSubWindow(nullptr), ctx(ctx), parent(nullptr), children() {}

            virtual bool NeedsSaving() = 0;
            virtual ntr::Result Save() = 0;

            virtual ntr::Result Import() = 0;
            virtual ntr::Result Export() = 0;

            void ShowChildWindow(SubWindow *child) {
                child->AssignParent(this);
                this->children.push_back(child);
                this->ctx->ShowSubWindow(child);
            }

            inline bool HasChildren() {
                return !this->children.empty();
            }

            bool CanClose() {
                for(auto &child: this->children) {
                    if(child->NeedsSaving()) {
                        QMessageBox::critical(this, "Window close", "Child window '" + child->windowTitle() + "' has unsaved changes...");
                        return false;
                    }
                }

                if(this->NeedsSaving()) {
                    const auto reply = QMessageBox::question(this, "Window close", "This window needs saving...\nWould you like to save the new changes in the original source?\n\nUse the exporting functionality if you wish to save in a new location.", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

                    if(reply == QMessageBox::Yes) {
                        return this->ctx->SaveFocusedSubWindow();
                    }
                    else if(reply == QMessageBox::No) {
                        return true;
                    }
                    else if(reply == QMessageBox::Cancel) {
                        return false;
                    }
                }

                return true;
            }

        protected:
            void closeEvent(QCloseEvent *event) override {
                if(!this->CanClose()) {
                    event->ignore();
                }
                else {
                    while(!this->children.empty()) {
                        this->children.front()->close();
                    }
                    this->children.clear();

                    if(this->parent != nullptr) {
                        this->parent->NotifyChildClosed(this);
                    }
                    this->ctx->NotifySubWindowClosed();

                    event->accept();
                }
            }

        protected:
            void AssignParent(SubWindow *parent_win) {
                this->parent = parent_win;
            }

            void NotifyChildClosed(SubWindow *child) {
                for(size_t i = 0; i < this->children.size(); i++) {
                    if(children.at(i) == child) {
                        this->children.erase(this->children.begin() + i);
                        return;
                    }
                }
            }

            Context *ctx;

            SubWindow *parent;
            std::vector<SubWindow*> children;
    };

    #define NEDIT_MOD_SYMBOL extern "C" Q_DECL_EXPORT

    #define NEDIT_MOD_INITIALIZE_SYMBOL __nedit_mod_Initialize
    using InitializeFunction = bool(*)(ModuleMetadata*);

    #define NEDIT_MOD_TRY_HANDLE_COMMAND_SYMBOL __nedit_mod_TryHandleCommand
    using TryHandleCommandFunction = bool(*)(const QString&, const QStringList&);

    #define NEDIT_MOD_TRY_HANDLE_INPUT_SYMBOL __nedit_mod_TryHandleInput
    using TryHandleInputFunction = bool(*)(const QString&, std::shared_ptr<ntr::fs::FileHandle>, Context*);

    #define NEDIT_MOD_DEFINE_START(m_name, m_desc, m_author, ver_major, ver_minor, ver_micro, ver_bugfix, rc_table_v, rc_table_size_v) \
        NEDIT_MOD_SYMBOL bool NEDIT_MOD_INITIALIZE_SYMBOL(::nedit::mod::ModuleMetadata &out_meta) { \
            out_meta.name = m_name; \
            out_meta.desc = m_desc; \
            out_meta.author = m_author; \
            out_meta.version.major = ver_major; \
            out_meta.version.minor = ver_minor; \
            out_meta.version.micro = ver_micro; \
            out_meta.version.bugfix = ver_bugfix; \
            out_meta.rc_table = rc_table_v; \
            out_meta.rc_table_size = rc_table_size_v; \
            return true; \
        } \
        NEDIT_MOD_SYMBOL bool NEDIT_MOD_TRY_HANDLE_COMMAND_SYMBOL(const QString &cmd, const QStringList &args) {

    #define NEDIT_MOD_DEFINE_REGISTER_COMMAND(name, handler) \
        if(cmd == name) { \
            const auto std_args = ::nedit::mod::ConvertQStringListToVector(args); \
            handler(std_args); \
            return true; \
        }

    #define NEDIT_MOD_DEFINE_END() \
            return false; \
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
