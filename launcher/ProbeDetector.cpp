#include "ProbeDetector.hpp"

#include "LauncherUtils.hpp"

#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <optional>

static bool libIsQtCore(const QByteArray &line)
{
    // TODO: в будущем необходимо учесть, что в разных ОС обозначение библиотек
    //  отличается. Сейчас ориентируемся только на Linux.
    QRegularExpression regex("^(.*)libQt(\\d+)Core\\.so(.*)$");
    QRegularExpressionMatch match = regex.match(line);
    return match.hasMatch();
}

static QString getQtCoreLibFromLdd(const QString &exePath)
{
    const auto depLibs = launcher::utils::getDependenciesForExecutable(exePath);
    for (const auto &lib : depLibs) {
        if (libIsQtCore(lib)) {
            return QString::fromUtf8(lib);
        }
    }
    return QString();
}

static std::optional<std::pair<int, int>> qtVersionFromLibName(const QString &libPath)
{
    QRegularExpression regex("^(.*)\\.so\\.(\\d+)\\.(\\d+)(?:\\.(\\d+))?$");
    QRegularExpressionMatch match = regex.match(libPath);

    if (!match.hasMatch()) {
        return std::nullopt;
    }

    const auto major = match.captured(2).toInt();
    const auto minor = match.captured(3).toInt();
    return std::make_pair(major, minor);
}

static std::optional<std::pair<int, int>> qtVersionFromLibExec(const QString &libPath)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::SeparateChannels);
    proc.setReadChannel(QProcess::StandardOutput);
    proc.start(libPath, {}, QProcess::ReadOnly);
    proc.waitForFinished();

    const QByteArray line = proc.readLine();
    const int pos = line.indexOf("Qt ");
    const QList<QByteArray> version = line.mid(pos + 2).split('.');
    if (version.size() < 3)
        return std::nullopt;

    return std::make_pair(version.at(0).toInt(), version.at(1).toInt());
}

namespace launcher::probe {
ProbeABI detectProbeAbiForExecutable(const QString &exePath)
{
    ProbeABI probe;

    const auto corePath = getQtCoreLibFromLdd(exePath);
    if (corePath.isEmpty()) {
        return probe;
    }

    QFileInfo coreFileInfo(corePath);
    if (!coreFileInfo.exists()) {
        return probe;
    }

    const auto canonicalCorePath = coreFileInfo.canonicalFilePath();
    auto qtVersion = qtVersionFromLibName(canonicalCorePath);
    if (!qtVersion.has_value()) {
        qtVersion = qtVersionFromLibExec(canonicalCorePath);
    }

    if (!qtVersion.has_value()) {
        return probe;
    }

    probe.setQtVersion(qtVersion.value());
    return probe;
}
}
