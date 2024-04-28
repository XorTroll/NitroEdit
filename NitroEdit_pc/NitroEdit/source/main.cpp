#include <ui/ui_MainWindow.hpp>
#include <mod/mod_Loader.hpp>
#include <QApplication>
#include <QTimer>
#include <QDebug>

namespace {

    int CliMain(QApplication &app) {
        auto args = QApplication::arguments();
        args.pop_front();

        const QString cmd = args.first();
        args.pop_front();

        auto ok = false;
        for(const auto &module: nedit::mod::GetModules()) {
            if(module.symbols.try_handle_cmd_fn(cmd, args)) {
                ok = true;
                break; 
            }
        }

        if(!ok) {
            qWarning() << "No match for command '" << cmd << "'... do you have the required module installed?";
        }

        QTimer::singleShot(0, &app, &QCoreApplication::quit);
        return app.exec();
    }

    int UiMain(QApplication &app) {
        ui::MainWindow win;
        win.show();

        return app.exec();
    }

}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    InitializeResults();
    nedit::mod::LoadModules();

    if(argc > 1) {
        return CliMain(app);
    }
    else {
        return UiMain(app);
    }
}
