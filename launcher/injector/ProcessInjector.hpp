#pragma once

#include "AbstractInjector.hpp"

namespace QtAda::launcher::injector {
class ProcessInjector : public AbstractInjector {
    Q_OBJECT
public:
    ProcessInjector() noexcept;
    ~ProcessInjector() noexcept override;

    void stop() noexcept override;
    int exitCode() noexcept override
    {
        return exitCode_;
    }
    QProcess::ExitStatus exitStatus() noexcept override
    {
        return exitStatus_;
    }
    QProcess::ProcessError processError() noexcept override
    {
        return processError_;
    }
    QString errorMessage() noexcept override
    {
        return errorMessage_;
    }

protected:
    bool injectAndLaunch(const QStringList &launchArgs, const QProcessEnvironment &env);

private slots:
    void processFinished() noexcept;
    void processFailed() noexcept;
    void readStdOutMessage() noexcept;
    void readStdErrMessage() noexcept;

private:
    QProcess process_;
    QProcess::ProcessError processError_ = QProcess::UnknownError;
    QProcess::ExitStatus exitStatus_ = QProcess::NormalExit;

    QString errorMessage_;
    int exitCode_ = -1;
};
} // namespace QtAda::launcher::injector
