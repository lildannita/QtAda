#include "launcher/Launcher.hpp"

#include <QDebug>
#include <QStringList>

void printUsage(const char *appPath)
{
    qInfo() << "Usage:" << appPath << "[options] <application> [args]";
    qInfo() << "";
    qInfo() << "Options: ";
    qInfo() << " -h, --help                       \tprint program help and exit";
    qInfo() << "";
}

int main(int argc, char *argv[])
{
    if (argc <= 1) {
        printUsage(*argv);
        return 1;
    }

    launcher::Launcher launcher;

    QStringList args;
    args.reserve(argc);
    for (int i = 1; i < argc; ++i)
        args.push_back(QString::fromLocal8Bit(argv[i]));

    while (!args.isEmpty() && args.first().startsWith('-')) {
        const auto arg = args.takeFirst();
        if ((arg == QLatin1String("-h")) || (arg == QLatin1String("--help"))) {
            printUsage(*argv);
        }
    }

    launcher.setLaunchAppArguments(args);
    if (!launcher.launch()) {
        qInfo() << qPrintable(QStringLiteral("Failed to launch target: %1").arg(args.constFirst()));
    }

    return 0;
}
