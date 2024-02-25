#pragma once

#include <QVector>

QT_BEGIN_NAMESPACE
class QByteArray;
class QString;
QT_END_NAMESPACE

namespace launcher::utils {
QVector<QByteArray> getDependenciesForExecutable(const QString &elfPath) noexcept;
QString absoluteExecutablePath(const QString &path) noexcept;
} // namespace launcher::utils
