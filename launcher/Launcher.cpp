#include "Launcher.hpp"

#include "ProbeDetector.hpp"

#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>

namespace launcher {
void Launcher::setProbeAbi(const probe::ProbeABI &probeAbi) noexcept
{
    options_.probe = std::move(probeAbi);
}

void Launcher::setLaunchAppArguments(const QStringList &args) noexcept
{
    options_.launchAppArguments = std::move(args);
}

QString Launcher::absoluteExecutablePath() const noexcept
{
    if (options_.launchAppArguments.isEmpty()) {
        return QString();
    }

    const auto originalExePath = options_.launchAppArguments.constFirst();
    const QFileInfo exeFileInfo(originalExePath);
    if (exeFileInfo.isFile() && exeFileInfo.isExecutable()) {
        return exeFileInfo.absoluteFilePath();
    }

    const auto exePath = QStandardPaths::findExecutable(options_.launchAppArguments.first());
    if (!exePath.isEmpty()) {
        return exePath;
    }

    return QString();
}

bool Launcher::launch() noexcept
{
    // AbstractInjector -> ProcessInjector -> PreloadInjector (get signals and exit Launcher)

    const auto exePath = absoluteExecutablePath();
    if (exePath.isEmpty()) {
        qInfo() << qPrintable(QStringLiteral("Error! %1: No such executable file.").arg(options_.launchAppArguments.constFirst()));
        return false;
    }

    const auto probe = probe::detectProbeAbiForExecutable(exePath);
    setProbeAbi(probe);

    if (!options_.probe.isValid()) {
        qInfo() << "Error! Can't detect ProbeABI.";
        return false;
    }

    const auto probeDll = options_.probe.probeDllPath();
    if (probeDll.isEmpty()) {
        qInfo() << qPrintable(QStringLiteral("Error! Can't locate probe for ABI '%1'.").arg(options_.probe.probeId()).toUtf8().constData());
        return false;
    }

    return true;
}
}
