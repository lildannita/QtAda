#pragma once

#include <QVector>

class QByteArray;
class QString;

namespace launcher::utils {
QVector<QByteArray> getDependenciesForExecutable(const QString &elfPath) noexcept;
QString absoluteExecutablePath(const QString &path) noexcept;
} // namespace launcher::utils
