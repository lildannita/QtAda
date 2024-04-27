#include <QApplication>
#include <QCoreApplication>
#include <QStringList>
#include <csignal>

#include "Common.hpp"
#include "Launcher.hpp"
#include "MainWindow.hpp"

namespace QtAda {
void shutdown(int sig)
{
    static volatile std::sig_atomic_t handlingSignal = 0;

    if (!handlingSignal) {
        handlingSignal = 1;
        printQtAdaOutMessage(QStringLiteral("Signal %1 received, shutting down.").arg(sig));
        QCoreApplication *app = QCoreApplication::instance();
        app->quit();
        return;
    }

    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

void installSignalHandler()
{
#ifdef SIGHUP
    std::signal(SIGHUP, shutdown);
#endif
#ifdef SIGINT
    std::signal(SIGINT, shutdown);
#endif
#ifdef SIGTERM
    std::signal(SIGTERM, shutdown);
#endif
}

int guiInitializer(int argc, char *argv[])
{
    QApplication app(argc, argv);

    gui::MainWindow mainWin;
    mainWin.show();

    auto exec = app.exec();
    return exec;
}

int cliInitializer(int argc, char *argv[])
{
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

    launcher::UserLaunchOptions options;
    const auto argsErrors = options.initFromArgs(*argv, std::move(args));
    if (argsErrors.has_value()) {
        return *argsErrors;
    }

    switch (options.type) {
    case LaunchType::Record: {
        QApplication app(argc, argv);
        launcher::Launcher launcher(options);
        if (launcher.launch()) {
            QObject::connect(&launcher, &launcher::Launcher::launcherFinished, &app,
                             &QCoreApplication::quit);
        }
        else {
            return launcher.exitCode();
        }

        auto exec = app.exec();
        return exec == 0 ? launcher.exitCode() : exec;
    }
    case LaunchType::Execute: {
        QCoreApplication app(argc, argv);
        launcher::Launcher launcher(options);
        if (launcher.launch()) {
            QObject::connect(&launcher, &launcher::Launcher::launcherFinished, &app,
                             &QCoreApplication::quit);
        }
        else {
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
    QtAda::installSignalHandler();
    return argc <= 1 ? QtAda::guiInitializer(argc, argv) : QtAda::cliInitializer(argc, argv);
}
