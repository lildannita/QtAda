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

// QString ProbeABI::probeDllPath() const noexcept
// {
//     const QString probePath = QDir::toNativeSeparators(QString::fromUtf8(QTADA_LIB_DIR))
//                               + QDir::separator() + QTADA_LIB_PREFIX + QTADA_PROBE_BASENAME +
//                               ".so";
//     QFileInfo probeInfo(probePath);
//     if (probeInfo.isFile() && probeInfo.isReadable()) {
//         return probeInfo.canonicalFilePath();
//     }
//     return QString();
// }

QString ProbeABI::probeDllPath() const noexcept
{
    const QString probeFileName = QString(QTADA_LIB_PREFIX) + QTADA_PROBE_BASENAME + ".so";

    // Список путей для поиска
    const QStringList searchPaths = {
        QString::fromUtf8(QTADA_LIB_DIR),
        QStringLiteral("/usr/lib"),
        QStringLiteral("/usr/lib64"),
        QStringLiteral("/usr/local/lib"),
        QStringLiteral("/usr/local/lib64"),
        QStringLiteral("/opt/qtada/lib"),
        QStringLiteral("/lib"),
        QStringLiteral("/lib64"),
        QDir::homePath() + QStringLiteral("/.local/lib"),
    };

    for (const QString &basePath : searchPaths) {
        if (basePath.isEmpty()) {
            continue;
        }

        const QString probePath
            = QDir::toNativeSeparators(basePath) + QDir::separator() + probeFileName;
        QFileInfo probeInfo(probePath);
        if (probeInfo.isFile() && probeInfo.isReadable()) {
            return probeInfo.canonicalFilePath();
        }
    }

    const QByteArray ldPath = qgetenv("LD_LIBRARY_PATH");
    if (!ldPath.isEmpty()) {
        const QStringList ldPaths = QString::fromLocal8Bit(ldPath).split(':');
        for (const QString &path : ldPaths) {
            const QString probePath
                = QDir::toNativeSeparators(path) + QDir::separator() + probeFileName;
            QFileInfo probeInfo(probePath);
            if (probeInfo.isFile() && probeInfo.isReadable()) {
                return probeInfo.canonicalFilePath();
            }
        }
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
