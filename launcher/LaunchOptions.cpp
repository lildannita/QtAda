#include "LaunchOptions.hpp"

#include "LauncherUtils.hpp"
#include "ProbeDetector.hpp"
#include "Common.hpp"

namespace QtAda::launcher {
static void printMultiplyDefinitionError()
{
    printQtAdaErrMessage(QStringLiteral("Multiply definition of application's launch type."));
}

static void printErrors(const std::vector<QString> &errors)
{
    for (const auto &error : errors) {
        printQtAdaErrMessage(error);
    }
}

static bool argToInt(int &option, const QString &value, const QString &arg) noexcept
{
    bool isOk = false;
    option = value.toInt(&isOk);
    if (!isOk) {
        printQtAdaErrMessage(QStringLiteral("Invalid value for %1.").arg(arg));
        return false;
    }
    return true;
}

std::optional<int> UserLaunchOptions::initFromArgs(const char *appPath, QStringList args) noexcept
{
    while (!args.isEmpty() && args.first().startsWith('-')) {
        const auto arg = args.takeFirst();
        if ((arg == QLatin1String("-h")) || (arg == QLatin1String("--help"))) {
            printUsage(appPath);
            return 0;
        }
        else if ((arg == QLatin1String("-w")) || (arg == QLatin1String("--workspace"))) {
            workingDirectory = std::move(args.takeFirst());
        }
        else if ((arg == QLatin1String("-t")) || (arg == QLatin1String("--timeout"))) {
            if (!argToInt(timeoutValue, args.takeFirst(), arg)) {
                return 1;
            }
            if (timeoutValue < DEFAULT_WAITING_TIMER_VALUE) {
                printQtAdaErrMessage(
                    QStringLiteral("Timeout value must be greater than %1 seconds.")
                        .arg(DEFAULT_WAITING_TIMER_VALUE));
            }
        }
        else if ((arg == QLatin1String("-r")) || (arg == QLatin1String("--record"))) {
            if (type != LaunchType::None) {
                printMultiplyDefinitionError();
                return 1;
            }
            type = LaunchType::Record;
            recordSettings.scriptPath = std::move(args.takeFirst());
            break;
        }
        else if ((arg == QLatin1String("-R")) || (arg == QLatin1String("--run"))) {
            if (type != LaunchType::None) {
                printMultiplyDefinitionError();
                return 1;
            }
            type = LaunchType::Run;
            runSettings.scriptPath = std::move(args.takeFirst());
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
        else if (arg == QLatin1String("--only-index")) {
            recordSettings.textIndexBehavior = TextIndexBehavior::OnlyIndex;
        }
        else if (arg == QLatin1String("--only-text")) {
            recordSettings.textIndexBehavior = TextIndexBehavior::OnlyText;
        }
        else if (arg == QLatin1String("--text-index")) {
            recordSettings.textIndexBehavior = TextIndexBehavior::TextIndex;
        }
        else if (arg == QLatin1String("--new-script")) {
            recordSettings.scriptWriteMode = ScriptWriteMode::NewScript;
        }
        else if (arg == QLatin1String("--update-script")) {
            recordSettings.scriptWriteMode = ScriptWriteMode::UpdateScript;
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
        else if (arg == QLatin1String("--attemps-number")) {
            if (!argToInt(runSettings.attempsNumber, args.takeFirst(), arg)) {
                return 1;
            }
        }
        else if (arg == QLatin1String("--retry-interval")) {
            if (!argToInt(runSettings.retryInterval, args.takeFirst(), arg)) {
                return 1;
            }
        }
        else {
            printQtAdaErrMessage(QStringLiteral("Unknown parameter: %1.").arg(arg));
            return 1;
        }
    }

    switch (type) {
    case LaunchType::Record: {
        const auto errors = recordSettings.isValid();
        if (errors.has_value()) {
            printErrors(*errors);
            return 1;
        }
        break;
    }
    case LaunchType::Run: {
        const auto errors = runSettings.isValid();
        if (errors.has_value()) {
            printErrors(*errors);
            return 1;
        }
        break;
    }
    case LaunchType::None: {
        printQtAdaErrMessage(QStringLiteral("Launch type is not specified."));
        return 1;
    }
    default:
        Q_UNREACHABLE();
    }
    launchAppArguments = std::move(args);

    if (launchAppArguments.isEmpty()) {
        printQtAdaErrMessage(QStringLiteral("Application path is not specified."));
        return 1;
    }

    return std::nullopt;
}

// Этот конструктор нужен только для MainGui::runAllScripts()
LaunchOptions::LaunchOptions(LaunchType type, bool fromGui) noexcept
{
    assert(type == LaunchType::Run);
    assert(fromGui == true);
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
