#include "LauncherUtils.hpp"

#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

namespace QtAda::launcher::utils {
static QVector<QByteArray> internalDependenciesGetter(const QString &elfPath, bool isRetry = false)
{
    QProcess ldProc;
    ldProc.setProcessChannelMode(QProcess::SeparateChannels);
    ldProc.setReadChannel(QProcess::StandardOutput);

    if (!isRetry) {
        // Сначала пробуем использовать ldd
        ldProc.start(QStringLiteral("ldd"), QStringList(elfPath));
        if (!ldProc.waitForStarted()) {
            return internalDependenciesGetter(elfPath, true);
        }
    }
    else {
        // Если ldd недоступен - то устанавливаем переменную окружения
        // LD_TRACE_LOADED_OBJECTS в единицу, тогда загрузчик Linux (ld.so) будет вести
        // себя как ldd и отобразит нужные зависимости
        QProcessEnvironment ldEnv = ldProc.processEnvironment();
        ldEnv.insert(QStringLiteral("LD_TRACE_LOADED_OBJECTS"), QStringLiteral("1"));
        ldProc.setProcessEnvironment(ldEnv);
        ldProc.start(elfPath, QStringList(), QProcess::ReadOnly);
    }
    ldProc.waitForFinished();

    QVector<QByteArray> deps;
    while (ldProc.canReadLine()) {
        const auto depLine = ldProc.readLine();
        if (depLine.isEmpty()) {
            break;
        }

        QRegularExpression regex("=>\\s*(.+?)\\s*\\(");
        QRegularExpressionMatch match = regex.match(depLine);
        if (match.hasMatch()) {
            deps.push_back(match.captured(1).toUtf8());
        }
    }

    return deps;
}

QVector<QByteArray> getDependenciesForExecutable(const QString &elfPath) noexcept
{
    return internalDependenciesGetter(elfPath);
}

QString absoluteExecutablePath(const QString &path) noexcept
{
    if (path.isEmpty()) {
        return QString();
    }

    const QFileInfo pathInfo(path);
    if (pathInfo.isFile() && pathInfo.isExecutable()) {
        return pathInfo.absoluteFilePath();
    }

    const auto exePath = QStandardPaths::findExecutable(path);
    if (!exePath.isEmpty()) {
        return exePath;
    }

    return QString();
}
} // namespace QtAda::launcher::utils
