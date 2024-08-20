#include "ProbeABI.hpp"

#include <QFileInfo>
#include <QDir>

#include <config.h>

namespace QtAda::launcher::probe {
void ProbeABI::setQtVersion(int major, int minor) noexcept
{
    info_.majorVersion = major;
    info_.minorQtVersion = minor;
}

void ProbeABI::setQtVersion(std::pair<int, int> version) noexcept
{
    setQtVersion(version.first, version.second);
}

void ProbeABI::setArchitecture(const QString architecture) noexcept
{
    info_.architecture = architecture;
}

bool ProbeABI::hasQtVersion() const noexcept
{
    return info_.majorVersion > 0 && info_.minorQtVersion >= 0;
}

bool ProbeABI::hasArchitecture() const noexcept
{
    return !info_.architecture.isEmpty();
}

bool ProbeABI::isValid() const noexcept
{
    return hasQtVersion() && hasArchitecture();
}

QString ProbeABI::probeDllPath() const noexcept
{
    const QString probePath = QDir::toNativeSeparators(QString::fromUtf8(QTADA_LIB_DIR))
                              + QDir::separator() + QTADA_LIB_PREFIX + QTADA_PROBE_BASENAME + ".so";
    QFileInfo probeInfo(probePath);
    if (probeInfo.isFile() && probeInfo.isReadable()) {
        return probeInfo.canonicalFilePath();
    }
    return QString();
}

QString ProbeABI::probeId() const noexcept
{
    if (!isValid()) {
        return QString();
    }

    return QStringLiteral("Qt %1.%2 (%3)")
        .arg(info_.majorVersion)
        .arg(info_.minorQtVersion)
        .arg(info_.architecture);
}
} // namespace QtAda::launcher::probe
