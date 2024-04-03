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

        std::vector<QString> avail_modules;
        for(const auto &module: nedit::mod::GetModules()) {
            if(module.symbols.provides_cmd_fn(cmd)) {
                avail_modules.push_back(module.meta.name);
            }
        }

        if(avail_modules.empty()) {
            qDebug() << "Unknown command...";
            return 1;
        }
        else if(avail_modules.size() > 1) {
            qDebug() << "Multiple modules serve this command:";
            for(const auto &mod: avail_modules) {
                qDebug() << "> " << mod << "";
            }
            qDebug() << "Cannot run due to ambiguity...";
            return 1;
        }
        else {
            for(const auto &module: nedit::mod::GetModules()) {
                if(module.symbols.provides_cmd_fn(cmd)) {
                    module.symbols.handle_cmd_fn(cmd, args); 
                }
            }
        }

        QTimer::singleShot(0, &app, &QCoreApplication::quit);
        return app.exec();
    }

    int UiMain(QApplication &app) {
        nedit::ui::MainWindow win;
        win.show();

        return app.exec();
    }

}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    nedit::mod::LoadModules();

    if(argc > 1) {
        return CliMain(app);
    }
    else {
        return UiMain(app);
    }
}
