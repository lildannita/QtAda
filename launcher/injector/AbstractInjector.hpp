#pragma once

#include <QObject>
#include <QProcess>

namespace launcher::injector {
class AbstractInjector : public QObject {
    Q_OBJECT
signals:
    void processStarted();
    void processFinished();

    void processStdoutMessage(const QString &message);
    void processStderrMessage(const QString &message);

public:
    ~AbstractInjector() override;

    virtual bool launch(const QStringList &launchArgs, const QString &probeDllPath);
    virtual void stop() = 0;
    virtual int exitCode() = 0;
    virtual QProcess::ExitStatus exitStatus() = 0;
    virtual QProcess::ProcessError processError() = 0;

    void setWorkingDirectory(const QString &dirPath) noexcept;
    QString workingDirectory() const noexcept;

private:
    QString workingDirectory_;
};
}
