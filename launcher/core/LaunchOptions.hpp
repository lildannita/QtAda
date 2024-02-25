#pragma once

#include "ProbeABI.hpp"

#include <QStringList>
#include <QProcessEnvironment>

namespace launcher {
enum LauncherState {
    Initial = 0,
    InjectorFinished = 1,
    InjectorFailed = 2,
    //    ClientStarted = 4,
    //    Complete = InjectorFinished | ClientStarted
};

struct UserLaunchOptions final {
    QStringList launchAppArguments;
    QString workingDirectory;

    bool isValid();
};

struct LaunchOptions final {
    UserLaunchOptions userOptions;
    QString absoluteExecutablePath;
    probe::ProbeABI probe;
    QProcessEnvironment env;

    uint8_t state = LauncherState::Initial;
    int exitCode = 0;

    explicit LaunchOptions(const UserLaunchOptions &options) noexcept;
};
} // namespace launcher
