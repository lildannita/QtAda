#include "AbstractInjector.hpp"

#include <QDir>

namespace launcher::injector {
AbstractInjector::~AbstractInjector() = default;

bool AbstractInjector::launch(const QStringList &launchArgs, const QString &probeDllPath)
{
    Q_UNUSED(launchArgs);
    Q_UNUSED(probeDllPath);
    return false;
}

void AbstractInjector::setWorkingDirectory(const QString &dirPath) noexcept
{
    workingDirectory_ = dirPath;
}

QString AbstractInjector::workingDirectory() const noexcept
{
    if (workingDirectory_.isEmpty()) {
        return QDir::currentPath();
    }
    return workingDirectory_;
}
}
