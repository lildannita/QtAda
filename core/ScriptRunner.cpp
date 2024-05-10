#include "ScriptRunner.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QThread>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QJSEngine>
#include <QQmlProperty>
#include <QDateTime>

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

bool ScriptRunner::writePropertyInGuiThread(QObject *object, const QString &propertyName,
                                            const QVariant &value) const noexcept
{
    assert(object != nullptr);
    auto func
        = [object, propertyName, value]() { QQmlProperty::write(object, propertyName, value); };
    return QMetaObject::invokeMethod(object, func, Qt::BlockingQueuedConnection);
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
    if (object == nullptr) {
        return;
    }

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

void ScriptRunner::sleep(int sec)
{
    QThread::sleep(sec);
}

void ScriptRunner::msleep(int msec)
{
    QThread::msleep(msec);
}

void ScriptRunner::usleep(int usec)
{
    QThread::usleep(usec);
}

void ScriptRunner::mouseClick(const QString &path, const QString &mouseButtonStr, int x,
                              int y) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

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
    if (object == nullptr) {
        return;
    }

    if (!object->inherits("QAbstractButton") && !object->inherits("QQuickAbstractButton")) {
        engine_->throwError(QStringLiteral("Passed object is not a button"));
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
    if (object == nullptr) {
        return;
    }

    const auto isWidgetButton = object->inherits("QAbstractButton");
    const auto isQuickButton = object->inherits("QQuickAbstractButton");

    if (!isWidgetButton && !isQuickButton) {
        engine_->throwError(QStringLiteral("Passed object is not a button"));
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
    if (object == nullptr) {
        return;
    }

    const auto isWidgetButton = object->inherits("QAbstractButton");
    const auto isQuickButton = object->inherits("QQuickAbstractButton");

    if (!isWidgetButton && !isQuickButton) {
        engine_->throwError(QStringLiteral("Passed object is not a button"));
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
    if (object == nullptr) {
        return;
    }

    if (!object->inherits("QQuickMouseArea")) {
        engine_->throwError(QStringLiteral("Passed object is not a mouse area"));
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
    if (object == nullptr) {
        return;
    }

    if (!object->inherits("QAbstractButton") && !object->inherits("QQuickAbstractButton")) {
        engine_->throwError(QStringLiteral("Passed object is not a button"));
        return;
    }
    if (!utils::getFromVariant<bool>(QQmlProperty::read(object, "checkable"))) {
        engine_->throwError(QStringLiteral("Button is not checkable"));
        return;
    }

    if (!checkObjectAvailability(object, path)) {
        return;
    }

    if (utils::getFromVariant<bool>(QQmlProperty::read(object, "checked")) == isChecked) {
        emit scriptWarning(QStringLiteral("'%1': button already has state '%2'")
                               .arg(path)
                               .arg(isChecked ? "true" : "false"));
    }
    else {
        bool ok = QMetaObject::invokeMethod(object, "toggle", Qt::BlockingQueuedConnection);
        assert(ok == true);
    }
}

void ScriptRunner::selectItemTemplate(const QString &path, int index, const QString &text,
                                      TextIndexBehavior behavior) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    const auto isWidgetComboBox = object->inherits("QComboBox");
    const auto isQuickComboBox = object->inherits("QQuickComboBox");
    const auto isQuickTumbler = object->inherits("QQuickTumbler");

    if (!isWidgetComboBox && !isQuickComboBox && !isQuickTumbler) {
        engine_->throwError(QStringLiteral("Passed object is not a combobox or tumbler"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    const auto count = utils::getFromVariant<int>(QQmlProperty::read(object, "count"));
    if (count == 0) {
        engine_->throwError(QStringLiteral("Passed object has no selectable items"));
        return;
    }

    auto indexHandler = [&](int currentIndex) {
        if (currentIndex >= count) {
            engine_->throwError(
                QStringLiteral("Index '%1' is out of range [0, %2)").arg(currentIndex).arg(count));
            return;
        }
        if (isWidgetComboBox) {
            bool ok = QMetaObject::invokeMethod(
                object, "setCurrentIndex", Qt::BlockingQueuedConnection, Q_ARG(int, currentIndex));
            assert(ok == true);
        }
        else if (isQuickComboBox || isQuickTumbler) {
            bool ok = writePropertyInGuiThread(object, "currentIndex", currentIndex);
            assert(ok == true);
        }
        else {
            Q_UNREACHABLE();
        }
    };

    auto indexFromText = [&] {
        int textIndex = -1;
        if (isWidgetComboBox) {
            bool ok = QMetaObject::invokeMethod(object, "findText", Qt::BlockingQueuedConnection,
                                                Q_RETURN_ARG(int, textIndex), Q_ARG(QString, text));
            assert(ok == true);
        }
        else if (isQuickComboBox) {
            bool ok = QMetaObject::invokeMethod(object, "find", Qt::BlockingQueuedConnection,
                                                Q_RETURN_ARG(int, textIndex), Q_ARG(QString, text));
            assert(ok == true);
        }
        else {
            Q_UNREACHABLE();
        }
        return textIndex;
    };

    switch (behavior) {
    case TextIndexBehavior::OnlyIndex: {
        indexHandler(index);
        break;
    }
    case TextIndexBehavior::OnlyText:
    case TextIndexBehavior::TextIndex: {
        if (isQuickTumbler) {
            engine_->throwError(QStringLiteral(
                "This QtAda version can't handle with text from this GUI component"));
            return;
        }

        auto textIndex = indexFromText();
        if (textIndex == -1) {
            switch (behavior) {
            case TextIndexBehavior::OnlyText: {
                engine_->throwError(QStringLiteral("Item with text '%1' does not exist").arg(text));
                break;
            }
            case TextIndexBehavior::TextIndex: {
                emit scriptWarning(QStringLiteral("'%1': item with text '%2' does not exist, "
                                                  "trying to use index '%3' instead")
                                       .arg(path)
                                       .arg(text)
                                       .arg(index));
                indexHandler(index);
                break;
            }
            default:
                Q_UNREACHABLE();
            }
            return;
        }
        assert(textIndex < count);
        indexHandler(textIndex);
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}

void ScriptRunner::selectItem(const QString &path, int index) const noexcept
{
    selectItemTemplate(path, index, QString(), TextIndexBehavior::OnlyIndex);
}

void ScriptRunner::selectItem(const QString &path, const QString &text) const noexcept
{
    selectItemTemplate(path, -1, text, TextIndexBehavior::OnlyText);
}

void ScriptRunner::selectItem(const QString &path, const QString &text, int index) const noexcept
{
    selectItemTemplate(path, index, text, TextIndexBehavior::TextIndex);
}

void ScriptRunner::setValue(const QString &path, double value) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    const auto isIntRequiringWidget
        = object->inherits("QAbstractSlider") || object->inherits("QSpinBox");
    const auto isDoubleRequiringWidget = object->inherits("QDoubleSpinBox");
    const auto isQuickSlider = object->inherits("QQuickSlider");
    const auto isQuickScrollBar = object->inherits("QQuickScrollBar");
    const auto isQuickSpinBox = object->inherits("QQuickSpinBox");

    if (!isIntRequiringWidget && !isDoubleRequiringWidget && !isQuickSlider && !isQuickScrollBar
        && !isQuickSpinBox) {
        engine_->throwError(QStringLiteral("This function doesn't support such an object"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    if (isIntRequiringWidget) {
        bool ok = QMetaObject::invokeMethod(object, "setValue", Qt::BlockingQueuedConnection,
                                            Q_ARG(int, value));
        assert(ok == true);
    }
    else if (isDoubleRequiringWidget) {
        bool ok = QMetaObject::invokeMethod(object, "setValue", Qt::BlockingQueuedConnection,
                                            Q_ARG(double, value));
        assert(ok == true);
    }
    else if (isQuickSlider) {
        bool ok = writePropertyInGuiThread(object, "value", value);
        assert(ok == true);
    }
    else if (isQuickScrollBar) {
        bool ok = writePropertyInGuiThread(object, "position", value);
        assert(ok == true);
    }
    else if (isQuickSpinBox) {
        int intValue;
        bool ok = QMetaObject::invokeMethod(object, "valueFromText", Qt::BlockingQueuedConnection,
                                            Q_RETURN_ARG(int, intValue),
                                            Q_ARG(QString, QString::number(value)));
        assert(ok == true);
        ok = writePropertyInGuiThread(object, "value", intValue);
        assert(ok == true);
    }
    else {
        Q_UNREACHABLE();
    }
}

void ScriptRunner::setValue(const QString &path, double leftValue, double rightValue) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    if (!object->inherits("QQuickRangeSlider")) {
        engine_->throwError(QStringLiteral("Passed object is not a range slider"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    bool ok = QMetaObject::invokeMethod(object, "setValues", Qt::BlockingQueuedConnection,
                                        Q_ARG(double, leftValue), Q_ARG(double, leftValue));
    assert(ok == true);
}

void ScriptRunner::setValue(const QString &path, const QString &value) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    const auto isWidgetCalendar = object->inherits("QCalendarView");
    const auto isWidgetEdit = object->inherits("QDateTimeEdit");
    const auto isQuickSpinBox = object->inherits("QQuickSpinBox");
    if (!isWidgetCalendar && !isWidgetEdit && !isQuickSpinBox) {
        engine_->throwError(QStringLiteral("This function doesn't support such an object"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    if (isWidgetEdit) {
        const auto dateTime = QDateTime::fromString(value, Qt::ISODate);
        if (dateTime.isNull() || dateTime.isValid()) {
            engine_->throwError(QStringLiteral("Can't convert '%1' to QDateTime").arg(value));
            return;
        }

        bool ok = false;
        if (dateTime.date().isNull()) {
            ok = QMetaObject::invokeMethod(object, "setTime", Qt::BlockingQueuedConnection,
                                           Q_ARG(QTime, dateTime.time()));
        }
        else if (dateTime.time().isNull()) {
            ok = QMetaObject::invokeMethod(object, "setDate", Qt::BlockingQueuedConnection,
                                           Q_ARG(QDate, dateTime.date()));
        }
        else {
            ok = QMetaObject::invokeMethod(object, "setDateTime", Qt::BlockingQueuedConnection,
                                           Q_ARG(QDateTime, dateTime));
        }
        assert(ok == true);
    }
    else if (isWidgetCalendar) {
        const auto date = QDate::fromString(value, Qt::ISODate);
        if (date.isNull() || date.isValid()) {
            engine_->throwError(QStringLiteral("Can't convert '%1' to QDate").arg(value));
            return;
        }
        bool ok = QMetaObject::invokeMethod(object, "setSelectedDate", Qt::BlockingQueuedConnection,
                                            Q_ARG(QDate, date));
        assert(ok == true);
    }
    else if (isQuickSpinBox) {
        int intValue;
        bool ok = QMetaObject::invokeMethod(object, "valueFromText", Qt::BlockingQueuedConnection,
                                            Q_RETURN_ARG(int, intValue), Q_ARG(QString, value));
        assert(ok == true);
        ok = writePropertyInGuiThread(object, "value", intValue);
        assert(ok == true);
    }
    else {
        Q_UNREACHABLE();
    }
}

void ScriptRunner::changeValue(const QString &path, const QString &type) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    const auto changeType = utils::changeTypeFromString(type);
    if (!changeType.has_value()) {
        engine_->throwError(QStringLiteral("Unknown change type: '%1'").arg(type));
        return;
    }

    const auto isWidgetSlider = object->inherits("QAbstractSlider");
    const auto isWidgetSpinBox = object->inherits("QSpinBox") || object->inherits("QDoubleSpinBox");
    const auto isQuickSpinBox = object->inherits("QQuickSpinBox");
    if (!isWidgetSlider && !isWidgetSpinBox && !isQuickSpinBox) {
        engine_->throwError(QStringLiteral("This function doesn't support such an object"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    if (isWidgetSlider) {
        bool isSliderType = true;
        auto value = utils::getFromVariant<int>(QQmlProperty::read(object, "value"));

        switch (*changeType) {
        case ChangeType::Up:
        case ChangeType::Down:
        case ChangeType::SingleStepAdd:
        case ChangeType::SingleStepSub: {
            const auto singleStep
                = utils::getFromVariant<int>(QQmlProperty::read(object, "singleStep"));
            value += (singleStep
                      * ((*changeType == ChangeType::Up || *changeType == ChangeType::SingleStepAdd)
                             ? 1
                             : -1));
            break;
        }
        case ChangeType::PageStepAdd:
        case ChangeType::PageStepSub: {
            const auto pageStep
                = utils::getFromVariant<int>(QQmlProperty::read(object, "pageStep"));
            value += pageStep * (*changeType == ChangeType::PageStepAdd ? 1 : -1);
            break;
        }
        case ChangeType::ToMinimum: {
            value = utils::getFromVariant<int>(QQmlProperty::read(object, "minimum"));
            break;
        }
        case ChangeType::ToMaximum: {
            value = utils::getFromVariant<int>(QQmlProperty::read(object, "maximum"));
            break;
        }
        default:
            Q_UNREACHABLE();
        }

        bool ok = QMetaObject::invokeMethod(object, "setValue", Qt::BlockingQueuedConnection,
                                            Q_ARG(int, value));
        assert(ok == true);
        return;
    }

    auto up = [&] {
        assert(isWidgetSpinBox == true || isQuickSpinBox == true);
        bool ok = QMetaObject::invokeMethod(object, isWidgetSpinBox ? "stepUp" : "increase",
                                            Qt::BlockingQueuedConnection);
        assert(ok == true);
    };
    auto down = [&] {
        assert(isWidgetSpinBox == true || isQuickSpinBox == true);
        bool ok = QMetaObject::invokeMethod(object, isWidgetSpinBox ? "stepDown" : "decrease",
                                            Qt::BlockingQueuedConnection);
        assert(ok == true);
    };

    switch (*changeType) {
    case ChangeType::Up: {
        up();
        break;
    }
    case ChangeType::DblUp: {
        up();
        up();
        break;
    }
    case ChangeType::Down: {
        down();
        break;
    }
    case ChangeType::DblDown: {
        down();
        down();
        break;
    }
    default:
        engine_->throwError(QStringLiteral("This object doesn't support such a change type"));
    }
}
} // namespace QtAda::core
