#include "Launcher.hpp"

#include "injector/PreloadInjector.hpp"

#include <QDebug>
#include <iostream>

static constexpr int DEFAULT_WAITING_TIMER_VALUE = 60;
static constexpr char ENV_TIMER_VAR_NAME[] = "QTADA_LAUNCHER_TIMEOUT";

namespace QtAda::launcher {
Launcher::Launcher(const UserLaunchOptions &userOptions, QObject *parent) noexcept
    : options_(std::move(userOptions))
{
    const auto userTimeoutValue = qgetenv(ENV_TIMER_VAR_NAME).toInt();
    waitingTimeoutValue_ = std::max(DEFAULT_WAITING_TIMER_VALUE, userTimeoutValue);
    waitingTimer_.setInterval(waitingTimeoutValue_ * 1000);
    waitingTimer_.setSingleShot(true);

    //! TODO: необходимо понять, как точно определить то, что приложение запущено,
    //! чтобы останавливать таймер
    // connect(&waitingTimer_, &QTimer::timeout, this, &Launcher::timeout);

    injector_ = std::make_unique<injector::PreloadInjector>();
    // connect(injector_.get(), &injector::AbstractInjector::started, this, &Launcher::restartTimer);
    connect(injector_.get(), &injector::AbstractInjector::finished, this, &Launcher::injectorFinished,
            Qt::QueuedConnection);
    connect(injector_.get(), &injector::AbstractInjector::stdErrMessage, this, &Launcher::printStdErrMessage);
    connect(injector_.get(), &injector::AbstractInjector::stdOutMessage, this, &Launcher::printStdOutMessage);
}

Launcher::~Launcher() noexcept
{
    if (injector_) {
        injector_->stop();
    }
}

void Launcher::timeout() noexcept
{
    options_.state |= LauncherState::InjectorFailed;
    qWarning() << qPrintable(QStringLiteral("Target not responding for %1 seconds. Try to setting the "
                                            "env variable '%2' to a bigger value (in seconds).")
                                 .arg(waitingTimeoutValue_)
                                 .arg(ENV_TIMER_VAR_NAME));

    checkIfLauncherIsFinished();
}

void Launcher::restartTimer() noexcept
{
    waitingTimer_.stop();
    waitingTimer_.start();
}

void Launcher::injectorFinished() noexcept
{
    assert(injector_ != nullptr);
    const auto errorMsg = injector_->errorMessage();
    if (!errorMsg.isEmpty()) {
        handleLauncherFailure(injector_->exitCode(), errorMsg);
        return;
    }

    options_.exitCode = injector_->exitCode();
    if ((options_.state & LauncherState::InjectorFailed) == 0) {
        options_.state |= InjectorFinished;
    }
    checkIfLauncherIsFinished();
}

bool Launcher::launch() noexcept
{
    if (options_.absoluteExecutablePath.isEmpty()) {
        handleLauncherFailure(-1, QStringLiteral("Error: '%1' - no such executable file.")
                                      .arg(options_.userOptions.launchAppArguments.constFirst()));
        return false;
    }

    if (!options_.probe.isValid()) {
        handleLauncherFailure(-1, QStringLiteral("Error: can't detect ProbeABI."));
        return false;
    }

    const auto probeDll = options_.probe.probeDllPath();
    if (probeDll.isEmpty()) {
        handleLauncherFailure(-1,
                              QStringLiteral("Error: can't locate probe for ABI '%1'.").arg(options_.probe.probeId()));
        return false;
    }

    injector_->setWorkingDirectory(options_.userOptions.workingDirectory);

    assert(injector_ != nullptr);
    //! TODO: добавить в env Qt библиотеки, если их не обнаружено
    if (!injector_->launch(options_.userOptions.launchAppArguments, probeDll, options_.env)) {
        QString errorMsg = QStringLiteral("Failed to launch target '%1'.")
                               .arg(options_.userOptions.launchAppArguments.join(QStringLiteral(" ")));
        const auto injectorErrorMsg = injector_->errorMessage();
        const auto injectorErrorCode = injector_->exitCode();
        if (!injectorErrorMsg.isEmpty()) {
            errorMsg.append(QStringLiteral("\nError: %1").arg(injectorErrorMsg));
        }
        handleLauncherFailure(injectorErrorCode ? injectorErrorCode : 1, errorMsg);
        return false;
    }

    return true;
}

void Launcher::handleLauncherFailure(int exitCode, const QString &errorMessage) noexcept
{
    options_.exitCode = exitCode;
    options_.state |= LauncherState::InjectorFailed;

    qCritical() << qPrintable(errorMessage);
    checkIfLauncherIsFinished();
}

void Launcher::checkIfLauncherIsFinished() noexcept
{
    if (options_.state == LauncherState::InjectorFinished) {
        emit launcherFinished();
    }
    else if ((options_.state & LauncherState::InjectorFailed) != 0) {
        if (options_.exitCode == 0) {
            options_.exitCode = 1;
        }
        emit launcherFinished();
    }
}

void Launcher::printStdOutMessage(const QString &msg) const noexcept
{
    std::cout << qPrintable(msg);
}

void Launcher::printStdErrMessage(const QString &msg) const noexcept
{
    std::cerr << qPrintable(msg);
}
} // namespace QtAda::launcher
