#pragma once

#include <QObject>
#include <QTimer>
#include <memory>

#include "LaunchOptions.hpp"

namespace QtAda::launcher::injector {
class AbstractInjector;
}

namespace QtAda::inprocess {
class InprocessDialog;
class InprocessRunner;
} // namespace QtAda::inprocess

namespace QtAda::launcher {
class Launcher final : public QObject {
    Q_OBJECT
public:
    explicit Launcher(LaunchType type, bool fromGui, QObject *parent = nullptr) noexcept;
    explicit Launcher(const UserLaunchOptions &options, bool fromGui = false,
                      QObject *parent = nullptr) noexcept;
    ~Launcher() noexcept override;

    void updateLaunchOptions(const UserLaunchOptions &options) noexcept;

    bool launch() noexcept;
    int exitCode() const noexcept
    {
        return options_.exitCode;
    }

    QString currentScriptPath() const noexcept
    {
        assert(options_.userOptions.type == LaunchType::Run);
        return options_.userOptions.runSettings.scriptPath;
    }

signals:
    void launcherFinished();

    void stdMessage(const QString &msg);
    void launcherOutMessage(const QString &msg);
    void launcherErrMessage(const QString &msg);

    void scriptRunError(const QString &msg);
    void scriptRunLog(const QString &msg);

private slots:
    void restartTimer() noexcept;
    void timeout() noexcept;
    void applicationStarted() noexcept;
    void injectorFinished() noexcept;

private:
    LaunchOptions options_;
    std::unique_ptr<injector::AbstractInjector> injector_;

    inprocess::InprocessDialog *inprocessDialog_ = nullptr;
    inprocess::InprocessRunner *inprocessRunner_ = nullptr;

    QTimer waitingTimer_;

    void checkIfLauncherIsFinished() noexcept;
    void handleLauncherFailure(int exitCode, const QString &errorMessage) noexcept;
    void destroyInprocessDialog() noexcept;

    void setInitialParameters(LaunchType type, bool fromGui) noexcept;
};
} // namespace QtAda::launcher
