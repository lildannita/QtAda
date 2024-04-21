#pragma once

#include "ProbeABI.hpp"
#include "LaunchOptions.hpp"

#include <QObject>
#include <QTimer>
#include <memory>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
QT_END_NAMESPACE

namespace QtAda::launcher::injector {
class AbstractInjector;
}

namespace QtAda::launcher {
class Launcher final : public QObject {
    Q_OBJECT
public:
    explicit Launcher(const UserLaunchOptions &options, QObject *parent = nullptr) noexcept;
    ~Launcher() noexcept override;

    bool launch() noexcept;
    int exitCode() const noexcept
    {
        return options_.exitCode;
    }

signals:
    void launcherFinished();

private slots:
    void restartTimer() noexcept;
    void timeout() noexcept;

    void injectorFinished() noexcept;
    void printStdOutMessage(const QString &msg) const noexcept;
    void printStdErrMessage(const QString &msg) const noexcept;

    void handleNewConnection() noexcept;

private:
    LaunchOptions options_;
    std::unique_ptr<injector::AbstractInjector> injector_;

    QLocalServer *server_ = nullptr;
    QLocalSocket *probeSocket_ = nullptr;
    QTimer waitingTimer_;
    int waitingTimeoutValue_ = 0;

    void checkIfLauncherIsFinished() noexcept;
    void handleLauncherFailure(int exitCode, const QString &errorMessage) noexcept;
};
} // namespace QtAda::launcher
