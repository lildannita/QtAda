#include "ProcessInjector.hpp"

namespace launcher::injector {
ProcessInjector::ProcessInjector() noexcept
{
    // Для того, чтобы не наршуть работу обработки ввода тестируемого приложения
    process_.setInputChannelMode(QProcess::ForwardedInputChannel);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(&process_, &QProcess::finished, this, &ProcessInjector::processFinished);
#else
    connect(
        &process_,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        this, &ProcessInjector::processFinished);
#endif
    connect(&process_, &QProcess::errorOccurred, this, &ProcessInjector::processFailed);

    connect(&process_, &QProcess::readyReadStandardError, this,
            &ProcessInjector::readStdErrMessage);
    connect(&process_, &QProcess::readyReadStandardOutput, this,
            &ProcessInjector::readStdOutMessage);
}

ProcessInjector::~ProcessInjector() noexcept { stop(); }

bool ProcessInjector::injectAndLaunch(const QStringList &launchArgs,
                                      const QProcessEnvironment &env)
{
    process_.setProcessEnvironment(env);
    process_.setWorkingDirectory(workingDirectory());

    QStringList args(launchArgs);
    const auto program = args.takeFirst();

    process_.start(program, args);
    bool isStarted = process_.waitForStarted(-1);
    if (isStarted) {
        emit started();
    }

    return isStarted;
}

void ProcessInjector::stop() noexcept
{
    // Так как останавливаем "вручную", то нам не важна обработка ошибок
    disconnect(&process_, &QProcess::errorOccurred, this,
               &ProcessInjector::processFailed);

    if (process_.state() != QProcess::Running)
        return;

    process_.terminate();
    if (!process_.waitForFinished(1000)) {
        // Если процесс не останавливается сам за 1 секунду, то вызываем kill SIGTERM
        process_.kill();
    }
}

void ProcessInjector::processFinished() noexcept
{
    exitStatus_ = process_.exitStatus();
    exitCode_ = process_.exitCode();

    if (processError_ == QProcess::FailedToStart) {
        errorMessage_.prepend(QStringLiteral("Failed to launch target '%1'.\nError: ")
                                  .arg(process_.program()));
    }

    emit finished();
}

void ProcessInjector::processFailed() noexcept
{
    processError_ = process_.error();
    errorMessage_ = process_.errorString();
}

void ProcessInjector::readStdOutMessage() noexcept
{
    const auto msg = process_.readAllStandardOutput();
    emit stdOutMessage(msg);
}

void ProcessInjector::readStdErrMessage() noexcept
{
    const auto msg = process_.readAllStandardError();
    emit stdErrMessage(msg);
}
} // namespace launcher::injector
