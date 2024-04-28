#include "Launcher.hpp"

#include <QApplication>
#include <QFile>

#include "injector/PreloadInjector.hpp"
#include "InprocessDialog.hpp"

#include "Common.hpp"

namespace QtAda::launcher {
Launcher::Launcher(const UserLaunchOptions &userOptions, bool fromGui, QObject *parent) noexcept
    : options_(std::move(userOptions))
{
    waitingTimer_.setInterval(options_.userOptions.timeoutValue * 1000);
    waitingTimer_.setSingleShot(true);

    connect(&waitingTimer_, &QTimer::timeout, this, &Launcher::timeout);

    injector_ = std::make_unique<injector::PreloadInjector>();
    connect(injector_.get(), &injector::AbstractInjector::started, this, &Launcher::restartTimer);
    connect(injector_.get(), &injector::AbstractInjector::finished, this,
            &Launcher::injectorFinished, Qt::QueuedConnection);

    if (fromGui) {
        connect(injector_.get(), &injector::AbstractInjector::stdMessage, this,
                &Launcher::stdMessage);
    }
    else {
        connect(injector_.get(), &injector::AbstractInjector::stdMessage, printStdMessage);
        connect(this, &Launcher::launcherErrMessage, printQtAdaErrMessage);
        connect(this, &Launcher::launcherOutMessage, printQtAdaOutMessage);
    }
}

Launcher::~Launcher() noexcept
{
    if (injector_ != nullptr) {
        injector_->stop();
    }
}

void Launcher::timeout() noexcept
{
    options_.state = LauncherState::InjectorFailed;
    emit launcherErrMessage(QStringLiteral("Target not responding for %1 seconds. Try to setting "
                                           "the timeout value to a bigger value (use --help).")
                                .arg(options_.userOptions.timeoutValue));
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

    options_.state = LauncherState::InjectorFinished;
    options_.exitCode = injector_->exitCode();
    checkIfLauncherIsFinished();
}

bool Launcher::launch() noexcept
{
    assert(injector_ != nullptr);
    assert(injector_->isLaunched() == false);

    //! TODO: добавить в env Qt библиотеки, если их не обнаружено

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
        handleLauncherFailure(-1, QStringLiteral("Error: can't locate probe for ABI '%1'.")
                                      .arg(options_.probe.probeId()));
        return false;
    }

    injector_->setWorkingDirectory(options_.userOptions.workingDirectory);

    options_.env.insert(ENV_LAUNCH_TYPE,
                        QString::number(static_cast<int>(options_.userOptions.type)));
    switch (options_.userOptions.type) {
    case LaunchType::Record: {
        options_.env.insert(ENV_LAUNCH_SETTINGS, options_.userOptions.recordSettings.toJson());
        inprocessDialog_ = new inprocess::InprocessDialog(options_.userOptions.recordSettings);
        connect(inprocessDialog_, &inprocess::InprocessDialog::applicationStarted, this, [this] {
            assert(waitingTimer_.isActive());
            waitingTimer_.stop();
        });
        connect(injector_.get(), &injector::AbstractInjector::stdMessage, inprocessDialog_,
                &inprocess::InprocessDialog::appendLogMessage);
        break;
    }
    case LaunchType::Execute: {
        options_.env.insert(ENV_LAUNCH_SETTINGS, options_.userOptions.executeSettings.toJson());
        //! TODO: здесь должно быть объявление класса, контролирующее запуск скрипта
        break;
    }
    default:
        Q_UNREACHABLE();
        break;
    }

    if (!injector_->launch(options_.userOptions.launchAppArguments, probeDll, options_.env)) {
        QString errorMsg
            = QStringLiteral("Failed to launch target '%1'.")
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
    options_.state = LauncherState::InjectorFailed;

    emit launcherErrMessage(errorMessage);
    checkIfLauncherIsFinished();
}

void Launcher::checkIfLauncherIsFinished() noexcept
{
    if (options_.state == LauncherState::InjectorFinished) {
        if (inprocessDialog_ != nullptr && inprocessDialog_->isStarted()) {
            inprocessDialog_->setTextToScriptLabel(
                QStringLiteral("The application under test has been closed. Please complete the "
                               "script generation in the dialog."));
            connect(inprocessDialog_, &inprocess::InprocessDialog::inprocessClosed, this,
                    &Launcher::launcherFinished);
        }
        else {
            emit launcherFinished();
        }
    }
    else if (options_.state == LauncherState::InjectorFailed) {
        if (options_.exitCode == 0) {
            options_.exitCode = 1;
        }
        destroyInprocessDialog();
        emit launcherFinished();
    }
}

void Launcher::destroyInprocessDialog() noexcept
{
    if (inprocessDialog_ == nullptr) {
        return;
    }
    inprocessDialog_->close();
    inprocessDialog_ = nullptr;
}
} // namespace QtAda::launcher
