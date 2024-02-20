#include "launcher/Launcher.hpp"
#include "launcher/ProbeDetector.hpp"

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


    const auto test = launcher::probe::detectProbeAbiForExecutable("/files/mgtu/android_currencyconverter/build-client-Desktop-Debug/client");

    return 0;
}
