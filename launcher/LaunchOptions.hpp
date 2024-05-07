#pragma once

#include <QStringList>
#include <QProcessEnvironment>

#include "ProbeABI.hpp"
#include "Settings.hpp"
#include "Common.hpp"

namespace QtAda::launcher {
enum class LauncherState {
    Initial = 0,
    InjectorFinished = 1,
    InjectorFailed = 2,
};

struct UserLaunchOptions final {
    QStringList launchAppArguments;
    QString workingDirectory;
    int timeoutValue = DEFAULT_WAITING_TIMER_VALUE;

    LaunchType type = LaunchType::None;
    RecordSettings recordSettings;
    QList<RunSettings> runSettings;

    std::optional<int> initFromArgs(const char *appPath, QStringList args) noexcept;
};

struct LaunchOptions final {
    UserLaunchOptions userOptions;
    QString absoluteExecutablePath;
    probe::ProbeABI probe;
    QProcessEnvironment env;

    QString runningScript;

    LauncherState state = LauncherState::Initial;
    int exitCode = 0;

    LaunchOptions(LaunchType type, bool fromGui) noexcept;
    explicit LaunchOptions(const UserLaunchOptions &options) noexcept;
};
} // namespace QtAda::launcher
