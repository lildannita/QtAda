#pragma once

#include "ProbeABI.hpp"

#include <QStringList>

namespace launcher {
class Launcher {
public:
    void setLaunchAppArguments(const QStringList &args) noexcept;
    bool launch() noexcept;

private:
    void setProbeAbi(const probe::ProbeABI &probe) noexcept;
    QString absoluteExecutablePath() const noexcept;

    struct {
        QStringList launchAppArguments;
        probe::ProbeABI probe;
    } options_;
};
}

