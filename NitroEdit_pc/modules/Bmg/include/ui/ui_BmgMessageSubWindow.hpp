
#pragma once
#include <base_Include.hpp>
#include <ui/ui_BmgSubWindow.hpp>

QT_BEGIN_NAMESPACE
namespace Ui {
    class BmgMessageSubWindow;
}
QT_END_NAMESPACE

namespace ui {

    class BmgMessageSubWindow : public nedit::mod::SubWindow {
        Q_OBJECT

        public:
            BmgMessageSubWindow(nedit::mod::Context *ctx, BmgSubWindow *parent_win, std::shared_ptr<ntr::fmt::BMG> bmg, const size_t idx);
            ~BmgMessageSubWindow();

            bool NeedsSaving() override;
            ntr::Result Save() override;

            ntr::Result Import() override;
            ntr::Result Export() override;

            inline size_t GetMessageIndex() {
                return this->msg_idx;
            }

        private:
            BmgSubWindow *parent_win;
            Ui::BmgMessageSubWindow *win_ui;
            std::shared_ptr<ntr::fmt::BMG> bmg_file;
            size_t msg_idx;
    };

}
