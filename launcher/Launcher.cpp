#include "Launcher.hpp"

#include <QApplication>
#include <QFile>

#include "injector/PreloadInjector.hpp"
#include "InprocessDialog.hpp"
#include "InprocessRunner.hpp"

#include "Common.hpp"

namespace QtAda::launcher {
Launcher::Launcher(const UserLaunchOptions &userOptions, bool fromGui, QObject *parent) noexcept
    : options_{ userOptions }
    , initFromGui_{ fromGui }
{
    const auto type = options_.userOptions.type;

    waitingTimer_.setInterval(options_.userOptions.timeoutValue * 1000);
    waitingTimer_.setSingleShot(true);
    connect(&waitingTimer_, &QTimer::timeout, this, &Launcher::timeout);

    injector_ = std::make_unique<injector::PreloadInjector>();
    connect(injector_.get(), &injector::AbstractInjector::started, this, &Launcher::restartTimer);
    connect(injector_.get(), &injector::AbstractInjector::finished, this,
            &Launcher::injectorFinished, Qt::QueuedConnection);

    if (initFromGui_) {
        connect(injector_.get(), &injector::AbstractInjector::stdMessage, this,
                &Launcher::stdMessage);
    }
    else {
        switch (type) {
        case LaunchType::Record: {
            connect(injector_.get(), &injector::AbstractInjector::stdMessage, printStdMessage);
            break;
        }
        case LaunchType::Run: {
            if (options_.userOptions.showAppLogForTestRun) {
                connect(injector_.get(), &injector::AbstractInjector::stdMessage, printStdMessage);
                connect(this, &Launcher::scriptRunError, printQtAdaErrorMessage);
                connect(this, &Launcher::scriptRunWarning, printQtAdaWarningMessage);
                connect(this, &Launcher::scriptRunLog, printQtAdaServiceMessage);
                connect(this, &Launcher::scriptRunService, printQtAdaServiceMessage);
                connect(this, &Launcher::scriptRunResult, printQtAdaServiceMessage);
            }
            else {
                connect(this, &Launcher::scriptRunError, printScriptErrorMessage);
                connect(this, &Launcher::scriptRunWarning, printScriptWarningMessage);
                connect(this, &Launcher::scriptRunLog, printScriptOutMessage);
                connect(this, &Launcher::scriptRunService, printScriptOutMessage);
                connect(this, &Launcher::scriptRunResult, printScriptOutMessage);
            }
            break;
        }
        default:
            Q_UNREACHABLE();
        }
        connect(this, &Launcher::launcherErrMessage, printQtAdaErrorMessage);
        connect(this, &Launcher::launcherOutMessage, printQtAdaOutMessage);
    }

    if (type == LaunchType::Run) {
        connect(this, &Launcher::launcherReadyForNextTest, this, [this] {
            const auto testedScript = options_.runningScript;
            assert(!testedScript.isEmpty());
            const auto code = exitCode();
            emit scriptFinished(code);
            if (code == 0) {
                scriptsRunData_.testsPassed++;
                emit scriptRunService(QStringLiteral("[       OK ] %1").arg(testedScript));
            }
            else {
                scriptsRunData_.testsFailed++;
                emit scriptRunService(QStringLiteral("[     FAIL ] %1").arg(testedScript));
            }

            if (options_.userOptions.runSettings.isEmpty()) {
                emit scriptRunResult("[==========]");
                if (scriptsRunData_.testsFailed > 0) {
                    options_.exitCode = 1;
                    emit scriptRunResult(QStringLiteral("[  FAILED  ] Passed: %1 | Failed: %2")
                                             .arg(scriptsRunData_.testsPassed)
                                             .arg(scriptsRunData_.testsFailed));
                }
                else {
                    emit scriptRunResult(
                        QStringLiteral("[  PASSED  ] %1 tests").arg(scriptsRunData_.testsPassed));
                }
                emit launcherFinished();
            }
            else {
                emit nextScriptStarted();
                launch();
            }
        });
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

void Launcher::applicationStarted() noexcept
{
    assert(waitingTimer_.isActive());
    waitingTimer_.stop();
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
        connect(inprocessDialog_, &inprocess::InprocessDialog::applicationStarted, this,
                &Launcher::applicationStarted);
        connect(injector_.get(), &injector::AbstractInjector::stdMessage, inprocessDialog_,
                &inprocess::InprocessDialog::appendLogMessage);
        break;
    }
    case LaunchType::Run: {
        const auto runSettings = options_.userOptions.runSettings.takeFirst();
        options_.runningScript = runSettings.scriptPath;

        // Пока только в GUI у нас есть возможность задать для каждого скрипта
        // свои аргументы запуска тестируемого приложения
        auto &launchAppArguments = options_.userOptions.launchAppArguments;
        assert(launchAppArguments.size() > 0);
        if (initFromGui_) {
            if (launchAppArguments.size() > 1) {
                launchAppArguments.erase(launchAppArguments.begin() + 1, launchAppArguments.end());
            }
            launchAppArguments.append(
                runSettings.executeArgs.split(QRegExp("\\s+"), Qt::SkipEmptyParts));
        }

        emit scriptRunService(QStringLiteral("[ RUN      ] %1").arg(options_.runningScript));
        options_.env.insert(ENV_LAUNCH_SETTINGS, runSettings.toJson());

        if (inprocessRunner_ == nullptr) {
            inprocessRunner_ = new inprocess::InprocessRunner(this);
            connect(inprocessRunner_, &inprocess::InprocessRunner::applicationStarted, this,
                    &Launcher::applicationStarted);
            connect(inprocessRunner_, &inprocess::InprocessRunner::scriptRunError, this,
                    &Launcher::scriptRunError);
            connect(inprocessRunner_, &inprocess::InprocessRunner::scriptRunWarning, this,
                    &Launcher::scriptRunWarning);
            connect(inprocessRunner_, &inprocess::InprocessRunner::scriptRunLog, this,
                    &Launcher::scriptRunLog);
        }
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
    bool isFinished = false;
    if (options_.state == LauncherState::InjectorFinished) {
        if (inprocessDialog_ != nullptr && inprocessDialog_->isStarted()) {
            connect(inprocessDialog_, &inprocess::InprocessDialog::inprocessClosed, this,
                    &Launcher::launcherFinished);
            inprocessDialog_->setApplicationClosedExternally();
            inprocessDialog_->setTextToScriptLabel(
                QStringLiteral("The application under test has been closed. Please complete the "
                               "script generation in the dialog."));
        }
        else {
            isFinished = true;
        }
    }
    else if (options_.state == LauncherState::InjectorFailed) {
        if (options_.exitCode == 0) {
            options_.exitCode = 1;
        }
        destroyInprocessDialog();
        isFinished = true;
    }

    if (!isFinished) {
        return;
    }

    if (options_.userOptions.type == LaunchType::Run) {
        emit launcherReadyForNextTest();
    }
    else {
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
