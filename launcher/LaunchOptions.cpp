#include "LaunchOptions.hpp"

#include "LauncherUtils.hpp"
#include "ProbeDetector.hpp"

namespace launcher {
bool UserLaunchOptions::isValid() { return !launchAppArguments.isEmpty(); }

LaunchOptions::LaunchOptions(const UserLaunchOptions &options) noexcept
    : userOptions(std::move(options))
    , env(QProcessEnvironment::systemEnvironment())
{
    assert(!options.launchAppArguments.isEmpty());

    absoluteExecutablePath
        = utils::absoluteExecutablePath(options.launchAppArguments.constFirst());
    probe = probe::detectProbeAbiForExecutable(absoluteExecutablePath);
}
} // namespace launcher
