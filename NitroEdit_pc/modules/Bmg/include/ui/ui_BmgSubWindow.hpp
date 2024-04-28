
#pragma once
#include <base_Include.hpp>

QT_BEGIN_NAMESPACE
namespace Ui {
    class BmgSubWindow;
}
QT_END_NAMESPACE

namespace ui {

    class BmgSubWindow : public nedit::mod::SubWindow {
        Q_OBJECT

        public:
            BmgSubWindow(nedit::mod::Context *ctx, std::shared_ptr<ntr::fmt::BMG> bmg, std::shared_ptr<ntr::fs::FileHandle> file_handle);
            ~BmgSubWindow();

            bool NeedsSaving() override;
            ntr::Result Save() override;

            ntr::Result Import() override;
            ntr::Result Export() override;

            void NotifyMessageEdited();
            void ReloadPreviews();

        private:
            ntr::fmt::BMG::Encoding GetCurrentEncoding();
            void ReloadEncoding();
            void OnMessageListViewDoubleClick(const QModelIndex &idx);

            Ui::BmgSubWindow *win_ui;
            std::shared_ptr<ntr::fmt::BMG> bmg_file;
            std::shared_ptr<ntr::fs::FileHandle> file_handle;
            bool msg_edited;
    };

}
