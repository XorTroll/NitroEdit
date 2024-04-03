
#pragma once
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

namespace nedit::ui {

    class MainWindow : public QMainWindow {
        Q_OBJECT

        public:
            MainWindow(QWidget *parent = nullptr);
            ~MainWindow();

        private:
            void OnOpenAction();

        private:
            Ui::MainWindow *win_ui;

    };

}
