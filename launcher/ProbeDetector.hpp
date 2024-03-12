#pragma once

#include "ProbeABI.hpp"

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace QtAda::launcher::probe {
ProbeABI detectProbeAbiForExecutable(const QString &elfPath) noexcept;
}
