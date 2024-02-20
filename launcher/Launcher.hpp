#pragma once

#include "ProbeABI.hpp"

#include <QStringList>

namespace launcher {
class Launcher {
public:
    void launch() const noexcept;

    void setProbeAbi(const probe::ProbeABI &probe) noexcept;
    void setLaunchAppArguments(const QStringList &args) noexcept;

private:
    QString getAbsoluteExecutablePath() const noexcept;
    QString getProbePath() const noexcept;

    bool optionsValid() const noexcept;

    struct {
        QStringList launchAppArguments;
        probe::ProbeABI probe;
    } options_;
};
}

