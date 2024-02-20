#include "LauncherUtils.hpp"

#include <QProcess>
#include <QRegularExpression>

static QVector<QByteArray> internalDependenciesGetter(const QString &elfPath, bool isRetry = false) {
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
        // Если ldd недоступен - то устанавливаем переменную окружения LD_TRACE_LOADED_OBJECTS
        //  в единицу, тогда загрузчик Linux (ld.so) будет вести себя как ldd и отобразит
        //   нужные зависимости
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

namespace launcher::utils {
QVector<QByteArray> getDependenciesForExecutable(const QString &elfPath) noexcept
{
    return internalDependenciesGetter(elfPath);
}
}
