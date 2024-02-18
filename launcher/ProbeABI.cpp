#include "ProbeABI.hpp"

namespace launcher::probe {
void ProbeABI::setQtVersion(int major, int minor) noexcept
{
    info_.majorVersion = major;
    info_.minorQtVersion = minor;
}

void ProbeABI::setQtVersion(std::pair<int, int> version) noexcept
{
    setQtVersion(version.first, version.second);
}

void ProbeABI::setArchitecture(const QString architecture) noexcept
{
    info_.architecture = architecture;
}

bool ProbeABI::hasQtVersion() const noexcept
{
    return info_.majorVersion > 0 && info_.minorQtVersion >= 0;
}

bool ProbeABI::hasArchitecture() const noexcept
{
    return !info_.architecture.isEmpty();
}

bool ProbeABI::isValid() const noexcept
{
    return hasQtVersion() && hasArchitecture();
}
}
