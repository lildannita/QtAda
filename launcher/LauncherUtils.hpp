#pragma once

#include <QVector>

QT_BEGIN_NAMESPACE
class QByteArray;
class QString;
QT_END_NAMESPACE

namespace QtAda::launcher::utils {
QVector<QByteArray> getDependenciesForExecutable(const QString &elfPath) noexcept;
QString absoluteExecutablePath(const QString &path) noexcept;
} // namespace QtAda::launcher::utils
