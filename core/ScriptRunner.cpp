#include "ScriptRunner.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QThread>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QJSEngine>
#include <QQmlProperty>

#include "utils/FilterUtils.hpp"
#include "utils/Tools.hpp"

namespace QtAda::core {
static QMouseEvent *simpleMouseEvent(const QEvent::Type type, const QPoint &pos,
                                     const Qt::MouseButton button = Qt::LeftButton) noexcept
{
    return new QMouseEvent(type, pos, button, button, Qt::NoModifier);
}

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

QObject *ScriptRunner::findObjectByPath(const QString &path) const noexcept
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

bool ScriptRunner::checkObjectAvailability(const QObject *object,
                                           const QString &path) const noexcept
{
    if (!utils::getFromVariant<bool>(QQmlProperty::read(object, "enabled"))) {
        emit scriptWarning(
            QStringLiteral("'%1': action on object ignored due to its disabled state").arg(path));
        return false;
    }
    else if (!utils::getFromVariant<bool>(QQmlProperty::read(object, "visible"))) {
        emit scriptWarning(
            QStringLiteral("'%1': action on object ignored due to its invisibility").arg(path));
        return false;
    }
    return true;
}

void ScriptRunner::verify(const QString &path, const QString &property,
                          const QString &value) const noexcept
{
    auto *object = findObjectByPath(path);

    QElapsedTimer timer;
    timer.start();

    const auto *metaObject = object->metaObject();
    const auto propertyIndex = metaObject->indexOfProperty(qPrintable(property));
    if (propertyIndex == -1) {
        engine_->throwError(QStringLiteral("Unknown meta-property name: '%1'").arg(property));
        return;
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
            return;
        }
        else {
            QThread::msleep(interval);
        }
    }
}

void ScriptRunner::mouseClick(const QString &path, const QString &mouseButtonStr, int x,
                              int y) const noexcept
{
    auto *object = findObjectByPath(path);
    const auto mouseButton = utils::mouseButtonFromString(mouseButtonStr);
    if (!mouseButton.has_value()) {
        engine_->throwError(QStringLiteral("Unknown mouse button: '%1'").arg(mouseButtonStr));
        return;
    }

    const auto pos = QPoint(x, y);
    QGuiApplication::postEvent(object,
                               simpleMouseEvent(QEvent::MouseButtonPress, pos, *mouseButton));
    QGuiApplication::postEvent(object,
                               simpleMouseEvent(QEvent::MouseButtonRelease, pos, *mouseButton));

    //! TODO: желательно отправлять события аналогично тому, как работает
    //! QMetaObject::invokeMethod(..., Qt::BlockingQueuedConnection);
    //! чтобы точно все "события" выполнялись по очереди. Но пока непонятно
    //! как это делать с обычными событиями, поэтому используем небольшую
    //! задержку:
    QThread::msleep(10);
    //! UPD: Можно определить собственный класс под такие события, который при
    //! "реализации" события, остановит QEventLoop, который был бы запущен сразу
    //! после очередного postEvent.
}

/*
 * На удивление, ситуация для воспроизведения нажатий, обратная обработке нажатий
 * (см. UserEventFilter::eventFilter).
 *
 * Примечание: для обычного Click в Quick и Widgets нужно в любом случае повторять цикл
 * P -> R, так как события Click не существует, ну или просто использовать метод toggle()
 * (но его нет для MouseArea в QML).
 *
 * В QML максимально просто можно эмулировать нужное нажатие на кнопку - нужно просто отправить
 * нужное событие.
 *
 * В QtWidgets для воспроизведения любого типа нажатия нужно воспроизвести полный цикл событий,
 * которые происходят "в оригинале" при генерации воспроизводимого события в GUI:
 *      1) Для DoubleClick: P -> R -> D -> R
 *      2) Для Press: P -> R-(вне области элемента)
 * В противном случае GUI не воспримет отправляемое событие или заблокирует обработку событий.
 */

void ScriptRunner::buttonClick(const QString &path) const noexcept
{
    auto *object = findObjectByPath(path);
    if (!object->inherits("QAbstractButton") && !object->inherits("QQuickAbstractButton")) {
        engine_->throwError(QStringLiteral("'%1': is not a button").arg(path));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    bool ok = QMetaObject::invokeMethod(object, "toggle", Qt::BlockingQueuedConnection);
    assert(ok == true);
}

void ScriptRunner::buttonDblClick(const QString &path) const noexcept
{
    auto *object = findObjectByPath(path);
    const auto isWidgetButton = object->inherits("QAbstractButton");
    const auto isQuickButton = object->inherits("QQuickAbstractButton");

    if (!isWidgetButton && !isQuickButton) {
        engine_->throwError(QStringLiteral("'%1': is not a button").arg(path));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    const auto height = utils::getFromVariant<double>(QQmlProperty::read(object, "height"));
    const auto width = utils::getFromVariant<double>(QQmlProperty::read(object, "width"));
    const auto pos = QPoint(width / 2, height / 2);
    if (isQuickButton) {
        QGuiApplication::postEvent(object, simpleMouseEvent(QEvent::MouseButtonDblClick, pos));
    }
    else if (isWidgetButton) {
        QGuiApplication::postEvent(object, simpleMouseEvent(QEvent::MouseButtonPress, pos));
        QGuiApplication::postEvent(object, simpleMouseEvent(QEvent::MouseButtonRelease, pos));
        QGuiApplication::postEvent(object, simpleMouseEvent(QEvent::MouseButtonDblClick, pos));
        QGuiApplication::postEvent(object, simpleMouseEvent(QEvent::MouseButtonRelease, pos));
    }
    else {
        Q_UNREACHABLE();
    }
}

void ScriptRunner::buttonPress(const QString &path) const noexcept
{
    auto *object = findObjectByPath(path);
    const auto isWidgetButton = object->inherits("QAbstractButton");
    const auto isQuickButton = object->inherits("QQuickAbstractButton");

    if (!isWidgetButton && !isQuickButton) {
        engine_->throwError(QStringLiteral("'%1': is not a button").arg(path));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    const auto height = utils::getFromVariant<double>(QQmlProperty::read(object, "height"));
    const auto width = utils::getFromVariant<double>(QQmlProperty::read(object, "width"));
    const auto pos = QPoint(width / 2, height / 2);
    if (isQuickButton) {
        QGuiApplication::postEvent(object, simpleMouseEvent(QEvent::MouseButtonPress, pos));
    }
    else if (isWidgetButton) {
        QGuiApplication::postEvent(object, simpleMouseEvent(QEvent::MouseButtonPress, pos));
        QGuiApplication::postEvent(object,
                                   simpleMouseEvent(QEvent::MouseButtonRelease, QPoint(-10, -10)));
    }
    else {
        Q_UNREACHABLE();
    }
}

void ScriptRunner::mouseAreaEventTemplate(const QString &path,
                                          const std::vector<QEvent::Type> events) const noexcept
{
    auto *object = findObjectByPath(path);

    if (!object->inherits("QQuickMouseArea")) {
        engine_->throwError(QStringLiteral("'%1': is not a mouse area").arg(path));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    const auto height = utils::getFromVariant<double>(QQmlProperty::read(object, "height"));
    const auto width = utils::getFromVariant<double>(QQmlProperty::read(object, "width"));
    const auto pos = QPoint(width / 2, height / 2);
    for (const auto &event : events) {
        QGuiApplication::postEvent(object, simpleMouseEvent(event, pos));
    }
}

void ScriptRunner::mouseAreaClick(const QString &path) const noexcept
{
    mouseAreaEventTemplate(path, { QEvent::MouseButtonPress, QEvent::MouseButtonRelease });
}

void ScriptRunner::mouseAreaDblClick(const QString &path) const noexcept
{
    mouseAreaEventTemplate(path, { QEvent::MouseButtonDblClick });
}

void ScriptRunner::mouseAreaPress(const QString &path) const noexcept
{
    mouseAreaEventTemplate(path, { QEvent::MouseButtonPress });
}

void ScriptRunner::checkButton(const QString &path, bool isChecked) const noexcept
{
    auto *object = findObjectByPath(path);

    if (!object->inherits("QAbstractButton") && !object->inherits("QQuickAbstractButton")) {
        engine_->throwError(QStringLiteral("'%1': is not a button").arg(path));
        return;
    }
    if (!utils::getFromVariant<bool>(QQmlProperty::read(object, "checkable"))) {
        engine_->throwError(QStringLiteral("'%1': button is not checkable").arg(path));
        return;
    }

    if (!checkObjectAvailability(object, path)) {
        return;
    }

    if (utils::getFromVariant<bool>(QQmlProperty::read(object, "checked")) == isChecked) {
        emit scriptWarning(
            QStringLiteral("'%1': button already has state %2").arg(isChecked ? "true" : "false"));
    }
    else {
        bool ok = QMetaObject::invokeMethod(object, "toggle", Qt::BlockingQueuedConnection);
        assert(ok == true);
    }
}

} // namespace QtAda::core
