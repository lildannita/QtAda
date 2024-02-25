#pragma once

#include "ProbeABI.hpp"

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace launcher::probe {
ProbeABI detectProbeAbiForExecutable(const QString &elfPath) noexcept;
}
