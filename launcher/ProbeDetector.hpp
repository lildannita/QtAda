#pragma once

#include "ProbeABI.hpp"

class QString;

namespace launcher::probe {
ProbeABI detectProbeAbiForExecutable(const QString &elfPath) noexcept;
}
