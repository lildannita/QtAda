#pragma once

#include <QVector>

#include "LauncherExport.hpp"

QT_BEGIN_NAMESPACE
class QByteArray;
class QString;
QT_END_NAMESPACE

namespace QtAda::launcher::utils {
QVector<QByteArray> getDependenciesForExecutable(const QString &elfPath) noexcept;
LAUNCHER_EXPORT QString absoluteExecutablePath(const QString &path) noexcept;
} // namespace QtAda::launcher::utils
