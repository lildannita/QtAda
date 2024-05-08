#include "ScriptRunner.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QThread>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QJSEngine>

#include "utils/FilterUtils.hpp"
#include "utils/Tools.hpp"

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
    if (oldPath != newPath) {
        pathToObject_.erase(oldPath);
        pathToObject_[newPath] = obj;
        it->second = newPath;
    }
}

void ScriptRunner::startScript() noexcept
{
    assert(this->thread() != qApp->thread());

    const auto &scriptPath = runSettings_.scriptPath;
    assert(!scriptPath.isEmpty());

    QFile script(scriptPath);
    assert(script.exists());

    const auto opened = script.open(QIODevice::ReadOnly);
    assert(opened == true);

    QTextStream stream(&script);
    const auto scriptContent = stream.readAll();
    script.close();

    if (scriptContent.trimmed().isEmpty()) {
        emit scriptError(QStringLiteral("{    ERROR    } Script is empty!").arg(scriptPath));
        finishThread(false);
        return;
    }

    engine_ = new QJSEngine(this);
    auto qtAdaJsObj = engine_->newQObject(this);
    engine_->globalObject().setProperty("QtAda", qtAdaJsObj);
    const auto runResult = engine_->evaluate(scriptContent);

    if (runResult.isError()) {
        emit scriptError(QStringLiteral("{    ERROR    } %1\n"
                                        "{ LINE NUMBER } %2\n"
                                        "{    STACK    }\n%3\n")
                             .arg(runResult.property("message").toString())
                             .arg(runResult.property("lineNumber").toInt())
                             .arg(runResult.property("stack").toString()));
        finishThread(false);
    }
    else {
        finishThread(true);
    }
}

void ScriptRunner::finishThread(bool isOk) noexcept
{
    emit aboutToClose(isOk ? 0 : 1);
}

QObject *ScriptRunner::findObjectByPath(const QString &path) noexcept
{
    QElapsedTimer timer;
    timer.start();

    const auto attempts = runSettings_.retrievalAttempts;
    assert(attempts >= MINIMUM_RETRIEVAL_ATTEMPTS);
    const auto interval = runSettings_.retrievalInterval;
    assert(interval >= MINIMUM_RETRIEVAL_INTERVAL);

    auto it = pathToObject_.end();
    for (int i = 0; i < attempts; i++) {
        it = pathToObject_.find(path);
        if (it != pathToObject_.end() && it->second != nullptr) {
            if (runSettings_.showElapsed) {
                auto elapsed = timer.elapsed();
                emit scriptLog(QStringLiteral("'%1' retrieved in %2 ms").arg(path).arg(elapsed));
            }
            return it->second;
        }
        if (i != attempts - 1) {
            QThread::msleep(interval);
        }
    }

    assert(it == pathToObject_.end());
    engine_->throwError(QStringLiteral("Failed to find the object at path '%1' "
                                       "after %2 attempts with an interval of %3 ms.")
                            .arg(path)
                            .arg(attempts)
                            .arg(interval));
    return nullptr;
}

void ScriptRunner::verify(const QString &path, const QString &property,
                          const QString &value) noexcept
{
    auto *object = findObjectByPath(path);

    QElapsedTimer timer;
    timer.start();

    const auto *metaObject = object->metaObject();
    const auto propertyIndex = metaObject->indexOfProperty(qPrintable(property));
    if (propertyIndex == -1) {
        engine_->throwError(QStringLiteral("Unknown meta-property name: '%1'").arg(property));
    }

    const auto attempts = runSettings_.verifyAttempts;
    assert(attempts >= MINIMUM_VERIFY_ATTEMPTS);
    const auto interval = runSettings_.verifyInterval;
    assert(interval >= MINIMUM_VERIFY_INTERVAL);

    for (int i = 0; i < attempts; i++) {
        const auto metaProperty = metaObject->property(propertyIndex);
        const auto currentValue = tools::metaPropertyValueToString(object, metaProperty);

        if (currentValue == value) {
            if (runSettings_.showElapsed) {
                const auto elapsed = timer.elapsed();
                emit scriptLog(QStringLiteral("'%1' verified in %2 ms").arg(path).arg(elapsed));
            }
            return;
        }

        if (i == attempts - 1) {
            engine_->throwError(QStringLiteral("Verify Failed!\n"
                                               "Object Path:      '%1'\n"
                                               "Property:         '%2'\n"
                                               "Expected Value:   '%3'\n"
                                               "Current Value:    '%4'\n"
                                               "Verify attempts:  '%5'\n"
                                               "Verify interval:  '%6'")
                                    .arg(path)
                                    .arg(property)
                                    .arg(value)
                                    .arg(currentValue)
                                    .arg(attempts)
                                    .arg(interval));
        }
        else {
            QThread::msleep(interval);
        }
    }
}

void ScriptRunner::mouseClick(const QString &path, const QString &mouseButtonStr, int x,
                              int y) noexcept
{
    auto *object = findObjectByPath(path);
    const auto mouseButton = utils::mouseButtonFromString(mouseButtonStr);
    if (!mouseButton.has_value()) {
        engine_->throwError(QStringLiteral("Unknown mouse button: '%1'").arg(mouseButtonStr));
        return;
    }

    const auto pos = QPoint(x, y);
    auto *pressEvent = new QMouseEvent(QEvent::MouseButtonPress, pos, *mouseButton, *mouseButton,
                                       Qt::NoModifier);
    auto *releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease, pos, *mouseButton,
                                         *mouseButton, Qt::NoModifier);

    QGuiApplication::postEvent(object, pressEvent);
    QGuiApplication::postEvent(object, releaseEvent);
}
} // namespace QtAda::core
