#include "launcher/Launcher.hpp"

#include <QDebug>
#include <QStringList>
#include <QCoreApplication>

#include <csignal>

void shutdownGracefully(int sig)
{
    static volatile std::sig_atomic_t handlingSignal = 0;

    if (!handlingSignal) {
        handlingSignal = 1;
        qDebug() << "Signal" << sig << "received, shutting down gracefully.";
        QCoreApplication *app = QCoreApplication::instance();
        app->quit();
        return;
    }

    // re-raise signal with default handler and trigger program termination
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

void installSignalHandler()
{
#ifdef SIGHUP
    std::signal(SIGHUP, shutdownGracefully);
#endif
#ifdef SIGINT
    std::signal(SIGINT, shutdownGracefully);
#endif
#ifdef SIGTERM
    std::signal(SIGTERM, shutdownGracefully);
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
    //    if (argc <= 1) {
    //        printUsage(*argv);
    //        return 1;
    //    }

    QCoreApplication app(argc, argv);

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
    //    args.push_back("/files/mgtu/android_currencyconverter/build-client-Desktop-Debug/client");
    options.launchAppArguments = std::move(args);

    launcher::Launcher launcher(options);
    if (launcher.launch()) {
        QObject::connect(&launcher, &launcher::Launcher::launcherFinished, &app,
                         &QCoreApplication::quit);
        //        qInfo() << qPrintable(
        //            QStringLiteral("Failed to launch target:
        //            %1").arg(options.launchAppArguments.constFirst());
    }
    auto exec = app.exec();

    return exec == 0 ? launcher.exitCode() : exec;
}
