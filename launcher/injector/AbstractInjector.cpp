#include "AbstractInjector.hpp"

#include <QDir>

namespace QtAda::launcher::injector {
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
} // namespace QtAda::launcher::injector
