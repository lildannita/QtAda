#include "AbstractInjector.hpp"

#include <QDir>

namespace QtAda::launcher::injector {
bool AbstractInjector::launch(const QStringList &launchArgs, const QString &probeDllPath,
                              const QProcessEnvironment &env) noexcept
{
    Q_UNUSED(launchArgs);
    Q_UNUSED(probeDllPath);
    Q_UNUSED(env);
    return false;
}

void AbstractInjector::setWorkingDirectory(const QString &dirPath) noexcept { workingDirectory_ = dirPath; }

QString AbstractInjector::workingDirectory() const noexcept
{
    if (workingDirectory_.isEmpty()) {
        return QDir::currentPath();
    }
    return workingDirectory_;
}
} // namespace QtAda::launcher::injector
