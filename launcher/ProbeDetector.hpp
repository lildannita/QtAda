#pragma once

#include "LauncherExport.hpp"
#include "ProbeABI.hpp"

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace QtAda::launcher::probe {
LAUNCHER_EXPORT ProbeABI detectProbeAbiForExecutable(const QString &elfPath) noexcept;
}
