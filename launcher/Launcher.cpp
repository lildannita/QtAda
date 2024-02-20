#include "Launcher.hpp"

#include <QFileInfo>
#include <QStandardPaths>

#include "config.h"

namespace launcher {
void Launcher::setProbeAbi(const probe::ProbeABI &probeAbi) noexcept
{
    options_.probe = std::move(probeAbi);
}

void Launcher::setLaunchAppArguments(const QStringList &args) noexcept
{
    options_.launchAppArguments = std::move(args);
}

QString Launcher::getAbsoluteExecutablePath() const noexcept
{
    if (options_.launchAppArguments.isEmpty()) {
        return QString();
    }

    const auto originalExePath = options_.launchAppArguments.first();
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

bool Launcher::optionsValid() const noexcept
{
    return options_.probe.isValid() && !getAbsoluteExecutablePath().isEmpty();
}

QString Launcher::getProbePath() const noexcept
{
    const auto probePath = QLatin1String(QTADA_LIB_DIR "/" QTADA_PROBE_BASENAME ".so");
    QFileInfo probeInfo(probePath);
    if (probeInfo.isFile() && probeInfo.isReadable(o)) {
        return probeInfo.canonicalFilePath();
    }
    return QString();
}

void Launcher::launch() const noexcept
{
    if (!optionsValid()) {
        // AbstractInjector -> ProcessInjector -> PreloadInjector (get signals and exit Launcher)
    }
}
}
