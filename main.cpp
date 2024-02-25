#include "launcher/Launcher.hpp"

#include <QDebug>
#include <QStringList>
#include <QCoreApplication>

#include <csignal>

void shutdown(int sig)
{
    static volatile std::sig_atomic_t handlingSignal = 0;

    if (!handlingSignal) {
        handlingSignal = 1;
        qInfo() << "Signal" << sig << "received, shutting down.";
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

void printUsage(const char *appPath)
{
    qInfo() << "Usage:" << appPath << "[options] <application> [args]";
    qInfo() << "";
    qInfo() << "Options: ";
    qInfo() << " -h, --help                       \tprint program help and exit";
    qInfo() << " -w, --workspace                  \tset working directory for executable";
    qInfo() << "";
}

int main(int argc, char *argv[])
{
    if (argc <= 1) {
        printUsage(*argv);
        return 1;
    }

    QCoreApplication app(argc, argv);
    installSignalHandler();

    QStringList args;
    args.reserve(argc);
    for (int i = 1; i < argc; ++i)
        args.push_back(QString::fromLocal8Bit(argv[i]));

    launcher::UserLaunchOptions options;
    while (!args.isEmpty() && args.first().startsWith('-')) {
        const auto arg = args.takeFirst();
        if ((arg == QLatin1String("-h")) || (arg == QLatin1String("--help"))) {
            printUsage(*argv);
            return 0;
        }
        if ((arg == QLatin1String("-w")) || (arg == QLatin1String("--workspace"))) {
            options.workingDirectory = std::move(args.takeFirst());
        }
    }
    options.launchAppArguments = std::move(args);

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
