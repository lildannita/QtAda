#include <QApplication>
#include <QCoreApplication>
#include <QStringList>
#include <csignal>

#include "Launcher.hpp"
#include "InitDialog.hpp"
#include "MainGui.hpp"

namespace QtAda {
int guiInitializer(int argc, char *argv[])
{
    using namespace gui;
    QApplication app(argc, argv);

    InitDialog startDialog;
    if (startDialog.exec() == QDialog::Accepted) {
        MainGui mainWindow(startDialog.selectedProjectPath());
        mainWindow.showMaximized();

        auto exec = app.exec();
        return exec;
    }

    return 0;
}

int cliInitializer(int argc, char *argv[])
{
    using namespace launcher;
    QStringList args;
    args.reserve(argc);
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (!arg.startsWith("-qmljsdebugger")) {
            args.push_back(arg);
        }
    }
    if (args.empty()) {
        return guiInitializer(argc, argv);
    }

    UserLaunchOptions options;
    const auto argsErrors = options.initFromArgs(*argv, std::move(args));
    if (argsErrors.has_value()) {
        return *argsErrors;
    }

    switch (options.type) {
    case LaunchType::Record: {
        QApplication app(argc, argv);
        Launcher launcher(options);
        QObject::connect(&launcher, &Launcher::launcherFinished, &app, &QCoreApplication::quit);
        if (!launcher.launch()) {
            return launcher.exitCode();
        }
        auto exec = app.exec();
        return exec == 0 ? launcher.exitCode() : exec;
    }
    case LaunchType::Run: {
        QCoreApplication app(argc, argv);
        Launcher launcher(options);
        QObject::connect(&launcher, &Launcher::launcherFinished, &app, &QCoreApplication::quit);
        if (!launcher.launch()) {
            return launcher.exitCode();
        }
        auto exec = app.exec();
        return exec == 0 ? launcher.exitCode() : exec;
    }
    default:
        Q_UNREACHABLE();
    }
}
} // namespace QtAda

int main(int argc, char *argv[])
{
    return argc <= 1 ? QtAda::guiInitializer(argc, argv) : QtAda::cliInitializer(argc, argv);
}
