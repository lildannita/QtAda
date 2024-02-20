#pragma once

#include <QVector>

class QByteArray;
class QString;

namespace launcher::utils {
QVector<QByteArray> getDependenciesForExecutable(const QString &elfPath);
}
