#include "ScriptRunner.hpp"

#include <QCoreApplication>

#include "utils/FilterUtils.hpp"

namespace QtAda::core {
ScriptRunner::ScriptRunner(const RunSettings &settings, QObject *parent) noexcept
    : QObject{ parent }
    , runSettings_{ settings }
{
}

//! TODO: большая проблема возникает из-за объектов графической оболочки -
//! мы не можем проверять уникальность данных в pathToObject_ и objectToPath_,
//! так как такие объекты могут быть разными указателями, но с одинаковыми путями.

void ScriptRunner::registerObjectCreated(QObject *obj) noexcept
{
    const auto path = utils::objectPath(obj);
    pathToObject_[path] = obj;
    objectToPath_[obj] = path;
}

void ScriptRunner::registerObjectDestroyed(QObject *obj) noexcept
{
    const auto it = objectToPath_.find(obj);
    if (it == objectToPath_.end()) {
        return;
    }

    const auto path = it->second;
    assert(pathToObject_.count(path) == 1);

    pathToObject_.erase(path);
    objectToPath_.erase(it);
}

void ScriptRunner::registerObjectReparented(QObject *obj) noexcept
{
    const auto newPath = utils::objectPath(obj);

    const auto it = objectToPath_.find(obj);
    if (it == objectToPath_.end()) {
        pathToObject_[newPath] = obj;
        objectToPath_[obj] = newPath;
        return;
    }

    const auto oldPath = it->second;
    assert(pathToObject_.count(oldPath) == 1);

    if (oldPath != newPath) {
        pathToObject_.erase(oldPath);
        pathToObject_[newPath] = obj;
        it->second = newPath;
    }
}

void ScriptRunner::startScript() noexcept
{
    assert(this->thread() != qApp->thread());
}
} // namespace QtAda::core
