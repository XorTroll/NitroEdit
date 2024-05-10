#include "./ui_ui_BmgSubWindow.h"
#include <ui/ui_BmgSubWindow.hpp>
#include <ui/ui_BmgMessageSubWindow.hpp>
#include <ntr/fs/fs_Stdio.hpp>
#include <QStringListModel>
#include <QFileDialog>
#include <QStringConverter>
#include <QRegularExpression>
#include <QFileInfo>

namespace ui {

    BmgSubWindow::BmgSubWindow(nedit::mod::Context *ctx, std::shared_ptr<ntr::fmt::BMG> bmg, std::shared_ptr<ntr::fs::FileHandle> file_handle) : SubWindow(ctx), win_ui(new Ui::BmgSubWindow), bmg_file(bmg), file_handle(file_handle), msg_edited(false) {
        this->win_ui->setupUi(this);

        this->win_ui->comboBoxEncoding->addItem(FormatEncoding(ntr::fmt::BMG::Encoding::CP1252));
        this->win_ui->comboBoxEncoding->addItem(FormatEncoding(ntr::fmt::BMG::Encoding::UTF16));
        this->win_ui->comboBoxEncoding->addItem(FormatEncoding(ntr::fmt::BMG::Encoding::ShiftJIS));
        this->win_ui->comboBoxEncoding->addItem(FormatEncoding(ntr::fmt::BMG::Encoding::UTF8));
        this->ReloadFields();

        this->win_ui->comboBoxEncoding->setToolTip("Change the encoding used with all messages");

        this->win_ui->lineEditFileId->setToolTip("Change the file ID");

        this->win_ui->listViewMessages->setToolTip("Double-click on a message to edit it");

        this->ReloadPreviews();
        connect(this->win_ui->listViewMessages, &QListView::doubleClicked, this, &BmgSubWindow::OnMessageListViewDoubleClick);
    }

    BmgSubWindow::~BmgSubWindow() {
        delete this->win_ui;
    }

    bool BmgSubWindow::NeedsSaving() {
        if(this->GetCurrentEncoding() != this->bmg_file->header.encoding) {
            return true;
        }

        ntr::u32 cur_file_id;
        if(!ParseStringInteger(this->win_ui->lineEditFileId->text(), cur_file_id)) {
            return true;
        }
        if(cur_file_id != this->bmg_file->info.file_id) {
            return true;
        }

        if(this->msg_edited) {
            return true;
        }

        return false;
    }

    ntr::Result BmgSubWindow::Save() {
        this->bmg_file->header.encoding = this->GetCurrentEncoding();
        
        if(!ParseStringInteger(this->win_ui->lineEditFileId->text(), this->bmg_file->info.file_id)) {
            NTR_R_FAIL(ResultBMGInvalidFileId);
        }

        NTR_R_TRY(this->bmg_file->WriteTo(this->file_handle));

        this->msg_edited = false;
        NTR_R_SUCCEED();
    }

    ntr::Result BmgSubWindow::Import() {
        QStringList filter_list;
        filter_list << NEDIT_MOD_BMG_FORMAT_BMG_FILTER;
        filter_list << NEDIT_MOD_BMG_FORMAT_XML_FILTER;
        const auto file = QFileDialog::getOpenFileName(this, "Import BMG from...", QString(), filter_list.join(";;"));

        if(!file.isEmpty()) {
            const auto file_ext = QFileInfo(file).suffix();
            if(file_ext == NEDIT_MOD_BMG_FORMAT_BMG_EXTENSION) {
                const auto rc = this->bmg_file->ReadFrom(file.toStdString(), this->file_handle);
                if(rc.IsFailure()) {
                    if(this->bmg_file->ReadFrom(file.toStdString(), this->file_handle, ntr::fs::FileCompression::LZ77).IsSuccess()) {
                        NTR_R_SUCCEED();
                    }

                    NTR_R_FAIL(rc);
                }
            }
            else if(file_ext == NEDIT_MOD_BMG_FORMAT_XML_EXTENSION) {
                NTR_R_TRY(LoadBmgXml(file, this->bmg_file));
            }

            this->ReloadFields();
            this->ReloadPreviews();
            this->msg_edited = true;
        }

        NTR_R_SUCCEED();
    }

    ntr::Result BmgSubWindow::Export() {
        QStringList filter_list;
        filter_list << NEDIT_MOD_BMG_FORMAT_BMG_FILTER;
        filter_list << NEDIT_MOD_BMG_FORMAT_XML_FILTER;
        const auto file = QFileDialog::getSaveFileName(this, "Export BMG as...", QString(), filter_list.join(";;"));

        if(!file.isEmpty()) {
            this->bmg_file->header.encoding = this->GetCurrentEncoding();

            const auto file_ext = QFileInfo(file).suffix();
            if(file_ext == NEDIT_MOD_BMG_FORMAT_BMG_EXTENSION) {
                NTR_R_TRY(this->bmg_file->WriteTo(file.toStdString(), this->file_handle));
            }
            else if(file_ext == NEDIT_MOD_BMG_FORMAT_XML_EXTENSION) {
                NTR_R_TRY(SaveBmgXml(this->bmg_file, file));
            }
        }

        NTR_R_SUCCEED();
    }

    void BmgSubWindow::NotifyMessageEdited() {
        this->msg_edited = true;
        this->ReloadPreviews();
    }

    void BmgSubWindow::ReloadPreviews() {
        QStringList msg_preview_list;
        for(const auto &msg: this->bmg_file->messages) {
            QString preview_text = "<no text preview>";
            for(const auto &token: msg.msg) {
                if(token.type == ntr::fmt::BMG::MessageTokenType::Text) {
                    preview_text = QString::fromStdU16String(token.text).split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts).front();
                    break;
                }
            }
            preview_text += "...";

            msg_preview_list << preview_text;
        }
        this->win_ui->listViewMessages->setModel(new QStringListModel(msg_preview_list));
    }

    ntr::fmt::BMG::Encoding BmgSubWindow::GetCurrentEncoding() {
        return static_cast<ntr::fmt::BMG::Encoding>(this->win_ui->comboBoxEncoding->currentIndex() + 1);
    }

    void BmgSubWindow::ReloadFields() {
        this->win_ui->comboBoxEncoding->setCurrentIndex(static_cast<size_t>(this->bmg_file->header.encoding) - 1);
        this->win_ui->lineEditFileId->setText(QString::number(this->bmg_file->info.file_id));
    }

    void BmgSubWindow::OnMessageListViewDoubleClick(const QModelIndex &idx) {
        const auto msg_idx = idx.row();

        for(auto &subwin: this->children) {
            auto msg_win = reinterpret_cast<BmgMessageSubWindow*>(subwin);
            if(msg_win->GetMessageIndex() == msg_idx) {
                return;
            }
        }

        QFileInfo file_info(QString::fromStdString(this->bmg_file->read_path));

        auto msg_win = new BmgMessageSubWindow(this->ctx, this, this->bmg_file, msg_idx);
        msg_win->setWindowTitle(QString("BMG message editor - %1[%2]").arg(file_info.fileName()).arg(QString::number(msg_idx)));
        this->ShowChildWindow(msg_win);
    }

}
