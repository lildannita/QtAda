#include "LaunchOptions.hpp"

#include "LauncherUtils.hpp"
#include "ProbeDetector.hpp"
#include "Common.hpp"

namespace QtAda::launcher {
static void printMultiplyDefinitionError()
{
    common::printQtAdaErrMessage(
        QStringLiteral("Multiply definition of application's launch type."));
}

static void printErrors(const std::vector<QString> &errors)
{
    for (const auto &error : errors) {
        common::printQtAdaErrMessage(error);
    }
}

static bool argToInt(int &option, const QString &value, const QString &arg) noexcept
{
    bool isOk = false;
    option = value.toInt(&isOk);
    if (!isOk) {
        common::printQtAdaErrMessage(QStringLiteral("Invalid value for %1.").arg(arg));
        return false;
    }
    return true;
}

std::optional<int> UserLaunchOptions::initFromArgs(const char *appPath, QStringList args) noexcept
{
    while (!args.isEmpty() && args.first().startsWith('-')) {
        const auto arg = args.takeFirst();
        if ((arg == QLatin1String("-h")) || (arg == QLatin1String("--help"))) {
            common::printUsage(appPath);
            return 0;
        }
        else if ((arg == QLatin1String("-w")) || (arg == QLatin1String("--workspace"))) {
            workingDirectory = std::move(args.takeFirst());
        }
        else if ((arg == QLatin1String("-t")) || (arg == QLatin1String("--timeout"))) {
            if (!argToInt(timeoutValue, args.takeFirst(), arg)) {
                return 1;
            }
            if (timeoutValue < common::DEFAULT_WAITING_TIMER_VALUE) {
                common::printQtAdaErrMessage(
                    QStringLiteral("Timeout value must be greater than %1 seconds.")
                        .arg(common::DEFAULT_WAITING_TIMER_VALUE));
            }
        }
        else if ((arg == QLatin1String("-r")) || (arg == QLatin1String("--record"))) {
            if (type != LaunchType::None) {
                printMultiplyDefinitionError();
                return 1;
            }
            type = launcher::LaunchType::Record;
            recordSettings.scriptPath = std::move(args.takeFirst());
            break;
        }
        else if ((arg == QLatin1String("-a")) || (arg == QLatin1String("--record-actions"))) {
            if (type != launcher::LaunchType::None) {
                printMultiplyDefinitionError();
                return 1;
            }
            type = launcher::LaunchType::RecordNoInprocess;
            recordSettings.scriptPath = std::move(args.takeFirst());
            recordSettings.inprocessDialog = false;
            break;
        }
        else if ((arg == QLatin1String("-e")) || (arg == QLatin1String("--execute"))) {
            if (type != launcher::LaunchType::None) {
                printMultiplyDefinitionError();
                return 1;
            }
            type = launcher::LaunchType::Execute;
            executeSettings.scriptPath = std::move(args.takeFirst());
            break;
        }
        else if (arg == QLatin1String("--indent-width")) {
            if (!argToInt(recordSettings.indentWidth, args.takeFirst(), arg)) {
                return 1;
            }
        }
        else if (arg == QLatin1String("--block-comment")) {
            if (!argToInt(recordSettings.blockCommentMinimumCount, args.takeFirst(), arg)) {
                return 1;
            }
        }
        else if (arg == QLatin1String("--duplicate-mouse-event")) {
            recordSettings.duplicateMouseEvent = true;
        }
        else if (arg == QLatin1String("--close-windows-on-exit")) {
            recordSettings.duplicateMouseEvent = true;
        }
        else if (arg == QLatin1String("--only-index")) {
            recordSettings.textIndexBehavior = common::TextIndexBehavior::OnlyIndex;
        }
        else if (arg == QLatin1String("--only-text")) {
            recordSettings.textIndexBehavior = common::TextIndexBehavior::OnlyText;
        }
        else if (arg == QLatin1String("--text-index")) {
            recordSettings.textIndexBehavior = common::TextIndexBehavior::TextIndex;
        }
        else if (arg == QLatin1String("--new-script")) {
            recordSettings.scriptWriteMode = common::ScriptWriteMode::NewScript;
        }
        else if (arg == QLatin1String("--update-script")) {
            recordSettings.scriptWriteMode = common::ScriptWriteMode::UpdateScript;
            if (!argToInt(recordSettings.appendLineIndex, args.takeFirst(), arg)) {
                return 1;
            }
        }
        else if (arg == QLatin1String("--generate-cycles")) {
            recordSettings.needToGenerateCycle = true;
        }
        else if (arg == QLatin1String("--cycle-min-count")) {
            if (!argToInt(recordSettings.cycleMinimumCount, args.takeFirst(), arg)) {
                return 1;
            }
        }
        else {
            common::printQtAdaErrMessage(QStringLiteral("Unknown parameter: %1.").arg(arg));
            return 1;
        }
    }

    switch (type) {
    case LaunchType::Record:
    case LaunchType::RecordNoInprocess: {
        const auto errors = recordSettings.isValid();
        if (errors.has_value()) {
            printErrors(*errors);
            return 1;
        }
        break;
    }
    case launcher::LaunchType::Execute: {
        const auto errors = executeSettings.isValid();
        if (errors.has_value()) {
            printErrors(*errors);
            return 1;
        }
        break;
    }
    case launcher::LaunchType::None: {
        common::printQtAdaErrMessage(QStringLiteral("Launch type is not specified."));
        return 1;
    }
    default:
        Q_UNREACHABLE();
    }
    launchAppArguments = std::move(args);

    return std::nullopt;
}

LaunchOptions::LaunchOptions(const UserLaunchOptions &options) noexcept
    : userOptions(std::move(options))
    , env(QProcessEnvironment::systemEnvironment())
{
    assert(!options.launchAppArguments.isEmpty());

    absoluteExecutablePath = utils::absoluteExecutablePath(options.launchAppArguments.constFirst());
    probe = probe::detectProbeAbiForExecutable(absoluteExecutablePath);
}
} // namespace QtAda::launcher
