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
    explicit Launcher(const UserLaunchOptions &options, bool fromGui = false,
                      QObject *parent = nullptr) noexcept;
    ~Launcher() noexcept override;

    bool launch() noexcept;
    int exitCode() const noexcept
    {
        return options_.exitCode;
    }

signals:
    void launcherFinished();
    void launcherReadyForNextTest();

    void stdMessage(const QString &msg);
    void launcherOutMessage(const QString &msg);
    void launcherErrMessage(const QString &msg);

    void scriptRunService(const QString &msg);
    void scriptRunResult(const QString &msg);
    void scriptRunError(const QString &msg);
    void scriptRunLog(const QString &msg);

private slots:
    void restartTimer() noexcept;
    void timeout() noexcept;
    void applicationStarted() noexcept;
    void injectorFinished() noexcept;

private:
    struct ScriptsRunData final {
        int testsPassed = 0;
        int testsFailed = 0;
    } scriptsRunData_;

    const bool initFromGui_;

    LaunchOptions options_;
    std::unique_ptr<injector::AbstractInjector> injector_;

    inprocess::InprocessDialog *inprocessDialog_ = nullptr;
    inprocess::InprocessRunner *inprocessRunner_ = nullptr;

    QTimer waitingTimer_;

    void checkIfLauncherIsFinished() noexcept;
    void handleLauncherFailure(int exitCode, const QString &errorMessage) noexcept;
    void destroyInprocessDialog() noexcept;
};
} // namespace QtAda::launcher
