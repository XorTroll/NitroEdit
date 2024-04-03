#include <ui/ui_MainWindow.hpp>
#include "./ui_ui_MainWindow.h"
#include <QMdiSubWindow>
#include <QMessageBox>
#include <ntr/util/util_String.hpp>

namespace nedit::ui {

    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), win_ui(new Ui::MainWindow) {
        this->win_ui->setupUi(this);

        connect(this->win_ui->openAction, &QAction::triggered, this, &MainWindow::OnOpenAction);
    }

    MainWindow::~MainWindow() {
        delete this->win_ui;
    }

    void MainWindow::OnOpenAction() {
        auto subwin = new QMdiSubWindow(this);
        subwin->setWindowTitle("Test sub");
        this->win_ui->mdiArea->addSubWindow(subwin);
        subwin->show();

        QMessageBox msgBox;
        auto demo = ntr::util::SplitString("aga", 'g');
        msgBox.setWindowTitle(QString(demo[0].c_str()));
        msgBox.setText("This is a simple text dialog.");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok);

        // Display the QMessageBox
        msgBox.exec();
    }

}
