#pragma once

#include "AbstractInjector.hpp"

namespace launcher::injector {
class ProcessInjector : public AbstractInjector {
    Q_OBJECT
public:
    ProcessInjector();
    ~ProcessInjector() override;

    bool launch(const QStringList &launchArgs, const QString &probeDllPath) override;
    void stop() override;
    int exitCode() override;
    QProcess::ExitStatus exitStatus() override;
    QProcess::ProcessError processError() override;

private:
    QProcess process_;
    QProcess::ProcessError processError_;
    QProcess::ExitStatus exitStatus_;

};
}
