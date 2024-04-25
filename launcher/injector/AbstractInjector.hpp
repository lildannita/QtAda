#pragma once

#include <QObject>
#include <QProcess>

/*
 * Рассчитываем на то, что в будущем будем обрабатывать действия не только
 * для новых процессов, но и для уже запущенных. Поэтому описываем оснвоное
 * поведение инъекторов через абстрактный класс.
 */

namespace QtAda::launcher::injector {
class AbstractInjector : public QObject {
    Q_OBJECT
public:
    ~AbstractInjector() override = default;

    virtual bool launch(const QStringList &launchArgs, const QString &probeDllPath,
                        const QProcessEnvironment &env) noexcept
        = 0;
    virtual void stop() = 0;
    virtual int exitCode() = 0;
    virtual QProcess::ExitStatus exitStatus() = 0;
    virtual QProcess::ProcessError processError() = 0;
    virtual QString errorMessage() = 0;
    virtual bool isLaunched() = 0;

    void setWorkingDirectory(const QString &dirPath) noexcept;
    QString workingDirectory() const noexcept;

signals:
    void started();
    void finished();
    void stdOutMessage(const QString &message);
    void stdErrMessage(const QString &message);

private:
    QString workingDirectory_;
};
} // namespace QtAda::launcher::injector
