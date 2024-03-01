#pragma once

#include "ProbeABI.hpp"
#include "LaunchOptions.hpp"

#include <QObject>
#include <QTimer>
#include <memory>

namespace QtAda::launcher {
namespace injector {
    class AbstractInjector;
} // namespace injector

class Launcher final : public QObject {
    Q_OBJECT
signals:
    void launcherFinished();

public:
    explicit Launcher(const UserLaunchOptions &options, QObject *parent = nullptr) noexcept;
    ~Launcher() noexcept override;

    bool launch() noexcept;
    int exitCode() const noexcept { return options_.exitCode; }

private slots:
    void restartTimer() noexcept;
    void timeout() noexcept;

    void injectorFinished() noexcept;
    void printStdOutMessage(const QString &msg) const noexcept;
    void printStdErrMessage(const QString &msg) const noexcept;

private:
    LaunchOptions options_;
    std::unique_ptr<injector::AbstractInjector> injector_;

    QTimer waitingTimer_;
    int waitingTimeoutValue_ = 0;

    void checkIfLauncherIsFinished() noexcept;
    void handleLauncherFailure(int exitCode, const QString &errorMessage) noexcept;
};
} // namespace QtAda::launcher
