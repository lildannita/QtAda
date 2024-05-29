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
#include <QModelIndex>
#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QLineEdit>
#include <QFuture>
#include <QTimer>
#include <QtConcurrent>
#include <QQmlEngine>

#include "utils/FilterUtils.hpp"
#include "utils/Tools.hpp"

namespace QtAda::core {
static constexpr int INVOKE_TIMEOUT_SEC = 3;

static QMouseEvent *simpleMouseEvent(const QEvent::Type type, const QPoint &pos,
                                     const Qt::MouseButton button = Qt::LeftButton) noexcept
{
    return new QMouseEvent(type, pos, button, button, Qt::NoModifier);
}

static std::vector<std::pair<QVariant, QVariant>> parseSelectionData(const QJSValue &selectionData,
                                                                     bool &isOk, int rowCount,
                                                                     int columnCount,
                                                                     QJSEngine *engine)
{
    std::vector<std::pair<QVariant, QVariant>> parsedData;

    if (!selectionData.isArray()) {
        isOk = false;
        engine->throwError(QStringLiteral("Passed selection data is not an array"));
        return parsedData;
    }

    const auto length = selectionData.property("length").toInt();
    if (length == 0) {
        isOk = false;
        engine->throwError(QStringLiteral("Passed selection data is empty"));
        return parsedData;
    }

    for (int i = 0; i < length; i++) {
        const auto entry = selectionData.property(i);
        QVariant rowVar, columnVar;
        const auto rowValue = entry.property("row").toString();
        const auto columnRawValue = entry.property("column");

        if (rowValue == "ALL") {
            rowVar = true;
        }
        else {
            bool ok;
            const auto row = rowValue.toInt(&ok);
            if (!ok) {
                isOk = false;
                engine->throwError(QStringLiteral("Passed selection data is invalid"));
                return parsedData;
            }
            else if (row < 0 || row >= rowCount) {
                isOk = false;
                engine->throwError(
                    QStringLiteral("Passed row index '%1' is out of row range [0, %2)")
                        .arg(row)
                        .arg(rowCount));
                return parsedData;
            }
            rowVar = row;
        }

        if (columnRawValue.toString() == "ALL") {
            columnVar = true;
        }
        else if (columnRawValue.isArray() || columnRawValue.isNumber()) {
            const auto isSingleDigit = columnRawValue.isNumber();
            QList<int> columns;
            const auto columnLength = isSingleDigit ? 1 : columnRawValue.property("length").toInt();
            if (columnLength == 0) {
                isOk = false;
                engine->throwError(QStringLiteral("Passed selection data is invalid"));
                return parsedData;
            }

            for (int j = 0; j < columnLength; j++) {
                bool ok;
                int column = -1;
                if (isSingleDigit) {
                    column = columnRawValue.toString().toInt(&ok);
                }
                else {
                    column = columnRawValue.property(j).toString().toInt(&ok);
                }
                if (!ok) {
                    isOk = false;
                    engine->throwError(QStringLiteral("Passed selection data is invalid"));
                    return parsedData;
                }
                else if (column < 0 || column >= columnCount) {
                    isOk = false;
                    engine->throwError(
                        QStringLiteral("Passed column index '%1' is out of column range [0, %2)")
                            .arg(column)
                            .arg(columnCount));
                    return parsedData;
                }
                columns.append(column);
            }
            columnVar = QVariant::fromValue(columns);
        }
        else {
            isOk = false;
            engine->throwError(QStringLiteral("Passed selection data is invalid"));
            return parsedData;
        }
        parsedData.push_back(std::make_pair(rowVar, columnVar));
    }
    return parsedData;
}

ScriptRunner::ScriptRunner(const RunSettings &settings, QObject *parent) noexcept
    : QObject{ parent }
    , runSettings_{ settings }
{
}

//! TODO: большая проблема возникает из-за объектов графической оболочки -
//! мы не можем проверять уникальность данных в pathToObject_ и objectToPath_,
//! так как такие объекты могут быть разными указателями, но с одинаковыми путями.

//! TODO: Для QtWidgets используется статический метод QQmlProperty::read/write,
//! причем название класса говорит о том, что по идее он предназначен для QML,
//! тем не менее он прекрасно работает и избавляет от нудных операций по типу:
//!     const QMetaObject* metaObject = object->metaObject();
//!     int propertyIndex = metaObject->indexOfProperty("....");
//!     QMetaProperty metaProperty = metaObject->property(propertyIndex);
//!     QVariant variant = metaProperty.read(object);
//!     ....
//! Но тем не менее нужно удостоверится, что все точно работает как надо.

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

//! TODO: Хоть эта функция вызывается только для Quick компонентов, для которых
//! по идее не должно быть проблемы с блокировкой текущего потока, нужно все равно
//! проверить, что проблем не будет
void ScriptRunner::writePropertyInGuiThread(QObject *object, const QString &propertyName,
                                            const QVariant &value) const noexcept
{
    assert(object != nullptr);
    auto func
        = [object, propertyName, value]() { QQmlProperty::write(object, propertyName, value); };
    bool ok = QMetaObject::invokeMethod(object, func, Qt::BlockingQueuedConnection);
    assert(ok == true);
}

void ScriptRunner::invokeBlockMethodWithTimeout(QObject *object, const char *method,
                                                QGenericArgument val0, QGenericArgument val1,
                                                QGenericArgument val2) const noexcept
{
    assert(object != nullptr);

    auto future = QtConcurrent::run([object, method, val0, val1, val2]() {
        bool ok = QMetaObject::invokeMethod(object, method, Qt::BlockingQueuedConnection, val0,
                                            val1, val2);
        assert(ok == true);
    });

    //! TODO: Конечно, лучше объявить эти данные в качестве поля данных в классе,
    //! но пока что они объявлены как const, в связи с чем пришлось бы убирать этот
    //! спецификатор. Позже нужно решить, что с этим делать
    QFutureWatcher<void> watcher;
    QEventLoop loop;
    QTimer timer;

    timer.setSingleShot(true);
    timer.start(INVOKE_TIMEOUT_SEC * 1000);

    QObject::connect(&watcher, &QFutureWatcher<void>::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    watcher.setFuture(future);

    loop.exec();

    if (timer.isActive()) {
        timer.stop();
        future.waitForFinished();
        assert(watcher.future().isFinished());
    }
    else {
        emit scriptWarning(QStringLiteral("Method '%1' took too long to execute (> %2 sec), "
                                          "stopping the wait for its completion")
                               .arg(method)
                               .arg(INVOKE_TIMEOUT_SEC));
    }
}

//! TODO: желательно отправлять события аналогично тому, как работает
//! QMetaObject::invokeMethod(..., Qt::BlockingQueuedConnection);
//! чтобы точно все "события" выполнялись по очереди. Но пока непонятно
//! как это делать с обычными событиями, поэтому используем небольшую
//! задержку: QThread::msleep(100);

void ScriptRunner::postEvents(QObject *object, std::vector<QEvent *> events) const noexcept
{
    assert(object != nullptr);
    for (auto *event : events) {
        QGuiApplication::postEvent(object, event);
    }
    QThread::msleep(100);
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

bool ScriptRunner::checkObjectAvailability(const QObject *object, const QString &path,
                                           bool shouldBeVisible) const noexcept
{
    const auto attempts = runSettings_.retrievalAttempts;
    assert(attempts >= MINIMUM_RETRIEVAL_ATTEMPTS);
    const auto interval = runSettings_.retrievalInterval;
    assert(interval >= MINIMUM_RETRIEVAL_INTERVAL);

    bool enabled = false;
    bool visible = shouldBeVisible ? false : true;

    //! TODO: позже нужно "встроить" это ожидание в функцию ScriptRunner::findObjectByPath
    for (int i = 0; i < attempts && (!enabled || !visible); i++) {
        enabled = utils::getFromVariant<bool>(QQmlProperty::read(object, "enabled"));
        if (shouldBeVisible) {
            visible = utils::getFromVariant<bool>(QQmlProperty::read(object, "visible"));
        }
        if (i != attempts - 1) {
            QThread::msleep(interval);
        }
    }

    if (!enabled) {
        emit scriptWarning(
            QStringLiteral("'%1': action on object ignored due to its disabled state").arg(path));
        return false;
    }
    else if (!visible) {
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

void ScriptRunner::mouseClickTemplate(const QString &path, const QString &mouseButtonStr, int x,
                                      int y, bool isDouble) const noexcept
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
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    const auto pos = QPoint(x, y);
    if (isDouble) {
        if (object->inherits("QQuickItem")) {
            postEvents(object,
                       { simpleMouseEvent(QEvent::MouseButtonDblClick, pos, *mouseButton) });
        }
        else {
            postEvents(object, { simpleMouseEvent(QEvent::MouseButtonPress, pos, *mouseButton),
                                 simpleMouseEvent(QEvent::MouseButtonRelease, pos, *mouseButton),
                                 simpleMouseEvent(QEvent::MouseButtonDblClick, pos, *mouseButton),
                                 simpleMouseEvent(QEvent::MouseButtonRelease, pos, *mouseButton) });
        }
    }
    else {
        postEvents(object, { simpleMouseEvent(QEvent::MouseButtonPress, pos, *mouseButton),
                             simpleMouseEvent(QEvent::MouseButtonRelease, pos, *mouseButton) });
    }
}

void ScriptRunner::mouseClick(const QString &path, const QString &mouseButtonStr, int x,
                              int y) const noexcept
{
    mouseClickTemplate(path, mouseButtonStr, x, y, false);
}

void ScriptRunner::mouseDblClick(const QString &path, const QString &mouseButtonStr, int x,
                                 int y) const noexcept
{
    mouseClickTemplate(path, mouseButtonStr, x, y, true);
}

/*
 * Для обычного click, в случае QAbstractButton достаточно использовать метод `click`,
 * но его нет для QQuickAbstractButton, поэтому для клика используются события P -> R.
 *
 * Вообще для QQuickAbstractButton есть метод `toggle`, похожий на клик, и по идее это он
 * и есть, но в QML при использовании метода `toggle` не будет вызван обработчик `onClicked`.
 */
void ScriptRunner::buttonClick(const QString &path) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    bool isWidgetButton = object->inherits("QAbstractButton");
    bool isQuickButton = object->inherits("QQuickAbstractButton");

    if (!isWidgetButton && !isQuickButton) {
        engine_->throwError(QStringLiteral("Passed object is not a button"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    if (isWidgetButton) {
        invokeBlockMethodWithTimeout(object, "click");
    }
    else {
        const auto height = utils::getFromVariant<double>(QQmlProperty::read(object, "height"));
        const auto width = utils::getFromVariant<double>(QQmlProperty::read(object, "width"));
        const auto pos = QPoint(width / 2, height / 2);
        postEvents(object, { simpleMouseEvent(QEvent::MouseButtonPress, pos),
                             simpleMouseEvent(QEvent::MouseButtonRelease, pos) });
    }
}

void ScriptRunner::buttonToggle(const QString &path) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    bool isWidgetButton = object->inherits("QAbstractButton");
    bool isQuickButton = object->inherits("QQuickAbstractButton");

    if (!isWidgetButton && !isQuickButton) {
        engine_->throwError(QStringLiteral("Passed object is not a button"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    invokeBlockMethodWithTimeout(object, "toggle");
}

/*
 * В QML максимально просто можно эмулировать двойное нажатие на кнопку - нужно просто отправить
 * соответствующее событие.
 *
 * В QtWidgets для воспроизведения любого типа нажатия нужно воспроизвести полный цикл событий,
 * которые происходят "в оригинале" при генерации воспроизводимого события в GUI:
 *      1) Для DoubleClick: P -> R -> D -> R
 *      2) Для Press: P -> R (вне области элемента)
 * В противном случае GUI не воспримет отправляемое событие или заблокирует обработку событий.
 */
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
        postEvents(object, { simpleMouseEvent(QEvent::MouseButtonDblClick, pos) });
    }
    else if (isWidgetButton) {
        postEvents(object, { simpleMouseEvent(QEvent::MouseButtonPress, pos),
                             simpleMouseEvent(QEvent::MouseButtonRelease, pos),
                             simpleMouseEvent(QEvent::MouseButtonDblClick, pos),
                             simpleMouseEvent(QEvent::MouseButtonRelease, pos) });
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
        postEvents(object, { simpleMouseEvent(QEvent::MouseButtonPress, pos) });
    }
    else if (isWidgetButton) {
        postEvents(object, { simpleMouseEvent(QEvent::MouseButtonPress, pos),
                             simpleMouseEvent(QEvent::MouseButtonRelease, QPoint(-10, -10)) });
    }
    else {
        Q_UNREACHABLE();
    }
}

void ScriptRunner::mouseAreaEventTemplate(const QString &path,
                                          const std::vector<QEvent::Type> eventTypes) const noexcept
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
    std::vector<QEvent *> events;
    for (const auto &event : eventTypes) {
        events.push_back(simpleMouseEvent(event, pos));
    }
    postEvents(object, events);
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
        invokeBlockMethodWithTimeout(object, "toggle");
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
        if (currentIndex < 0 || currentIndex >= count) {
            engine_->throwError(
                QStringLiteral("Index '%1' is out of range [0, %2)").arg(currentIndex).arg(count));
            return;
        }
        if (isWidgetComboBox) {
            invokeBlockMethodWithTimeout(object, "setCurrentIndex", Q_ARG(int, currentIndex));
        }
        else if (isQuickComboBox || isQuickTumbler) {
            writePropertyInGuiThread(object, "currentIndex", currentIndex);
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

void ScriptRunner::setValueIntoQmlSpinBox(QObject *object, const QString &value) const noexcept
{
    assert(object != nullptr);

    auto standardWayToSetValue = [&] {
        bool ok = false;
        int intValue = value.toInt(&ok);
        if (!ok) {
            engine_->throwError(
                QStringLiteral("Cannot convert '%1' to an integer (required for QML SpinBox)")
                    .arg(value));
        }
        else {
            writePropertyInGuiThread(object, "value", intValue);
        }
    };

    auto issueWarningAndFallback = [&](const QString &warningMessage) {
        emit scriptWarning(warningMessage);
        standardWayToSetValue();
    };

    auto rawValueFromText = QQmlProperty::read(object, "valueFromText");
    if (rawValueFromText.canConvert<QJSValue>()) {
        auto valueFromText = rawValueFromText.value<QJSValue>();
        if (valueFromText.isCallable()) {
            auto *originalEngine = qmlEngine(object);
            assert(originalEngine != nullptr);
            auto locale = QLocale::system();
            auto jsLocale = originalEngine->toScriptValue(locale);

            auto jsValue = value;
            bool isDouble = false;
            auto doubleValue = value.toDouble(&isDouble);
            if (isDouble) {
                jsValue = locale.toString(doubleValue);
            }

            auto rawValue = valueFromText.call(QJSValueList() << jsValue << jsLocale);
            if (!rawValue.isError() && rawValue.isNumber()) {
                writePropertyInGuiThread(object, "value", rawValue.toInt());
            }
            else {
                issueWarningAndFallback("Invalid result from `valueFromText` for QML SpinBox. "
                                        "Setting original text value...");
            }
        }
        else {
            issueWarningAndFallback("`valueFromText` for QML SpinBox is not callable. Setting "
                                    "original text value...");
        }
    }
    else {
        issueWarningAndFallback("Unable to retrieve `valueFromText` function for QML SpinBox. "
                                "Setting original text value...");
    }
}

void ScriptRunner::setValueTemplate(QObject *object, const QString &path,
                                    double value) const noexcept
{
    assert(object != nullptr);

    const auto isIntRequiringWidget
        = object->inherits("QAbstractSlider") || object->inherits("QSpinBox");
    const auto isDoubleRequiringWidget = object->inherits("QDoubleSpinBox");
    const auto isQuickSlider = object->inherits("QQuickSlider");
    const auto isQuickScrollBar = object->inherits("QQuickScrollBar");
    const auto isQuickSpinBox = object->inherits("QQuickSpinBox");
    const auto isQuickDial = object->inherits("QQuickDial");

    if (!isIntRequiringWidget && !isDoubleRequiringWidget && !isQuickSlider && !isQuickScrollBar
        && !isQuickSpinBox && !isQuickDial) {
        engine_->throwError(QStringLiteral("This function doesn't support such an object"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    if (isIntRequiringWidget) {
        invokeBlockMethodWithTimeout(object, "setValue", Q_ARG(int, value));
    }
    else if (isDoubleRequiringWidget) {
        invokeBlockMethodWithTimeout(object, "setValue", Q_ARG(double, value));
    }
    else if (isQuickSlider || isQuickDial) {
        writePropertyInGuiThread(object, "value", value);
    }
    else if (isQuickScrollBar) {
        writePropertyInGuiThread(object, "position", value);
    }
    else if (isQuickSpinBox) {
        setValueIntoQmlSpinBox(object, QString::number(value));
    }
    else {
        Q_UNREACHABLE();
    }
}

void ScriptRunner::setValue(const QString &path, double value) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }
    setValueTemplate(object, path, value);
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
    invokeBlockMethodWithTimeout(object, "setValues", Q_ARG(double, leftValue),
                                 Q_ARG(double, rightValue));
}

void ScriptRunner::setValueTemplate(QObject *object, const QString &path,
                                    const QString &value) const noexcept
{
    assert(object != nullptr);

    const auto isWidgetCalendar = object->inherits("QCalendarWidget");
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
        const auto time = QTime::fromString(value, Qt::ISODate);
        const auto timeValid = !time.isNull() && time.isValid();
        const auto date = QDate::fromString(value, Qt::ISODate);
        const auto dateValid = !date.isNull() && date.isValid();
        const auto dateTime = QDateTime::fromString(value, Qt::ISODate);
        const auto dateTimeValid = !dateTime.isNull() && dateTime.isValid();

        if (!dateTimeValid && !timeValid && !dateValid) {
            engine_->throwError(
                QStringLiteral("Can't convert '%1' to QDateTime (or QDate, or QTime)").arg(value));
            return;
        }
        if (dateTimeValid) {
            invokeBlockMethodWithTimeout(object, "setDateTime", Q_ARG(QDateTime, dateTime));
        }
        else if (timeValid) {
            invokeBlockMethodWithTimeout(object, "setTime", Q_ARG(QTime, time));
        }
        else if (dateValid) {
            invokeBlockMethodWithTimeout(object, "setDate", Q_ARG(QDate, date));
        }
        else {
            Q_UNREACHABLE();
        }
    }
    else if (isWidgetCalendar) {
        const auto date = QDate::fromString(value, Qt::ISODate);
        if (date.isNull() || !date.isValid()) {
            engine_->throwError(QStringLiteral("Can't convert '%1' to QDate").arg(value));
            return;
        }
        invokeBlockMethodWithTimeout(object, "setSelectedDate", Q_ARG(QDate, date));
    }
    else if (isQuickSpinBox) {
        setValueIntoQmlSpinBox(object, value);
    }
    else {
        Q_UNREACHABLE();
    }
}

void ScriptRunner::setValue(const QString &path, const QString &value) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }
    setValueTemplate(object, path, value);
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

        invokeBlockMethodWithTimeout(object, "setValue", Q_ARG(int, value));
        return;
    }

    auto up = [&] {
        assert(isWidgetSpinBox == true || isQuickSpinBox == true);
        invokeBlockMethodWithTimeout(object, isWidgetSpinBox ? "stepUp" : "increase");
    };
    auto down = [&] {
        assert(isWidgetSpinBox == true || isQuickSpinBox == true);
        invokeBlockMethodWithTimeout(object, isWidgetSpinBox ? "stepDown" : "decrease");
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

void ScriptRunner::setDelayProgress(const QString &path, double delay) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    if (!object->inherits("QQuickDelayButton")) {
        engine_->throwError(QStringLiteral("Passed object is not a delay button"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }
    writePropertyInGuiThread(object, "progress", delay);
}

void ScriptRunner::selectTabItemTemplate(const QString &path, int index, const QString &text,
                                         TextIndexBehavior behavior) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    if (!object->inherits("QTabBar")) {
        engine_->throwError(QStringLiteral("Passed object is not a tab widget"));
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
        if (currentIndex < 0 || currentIndex >= count) {
            engine_->throwError(
                QStringLiteral("Index '%1' is out of range [0, %2)").arg(currentIndex).arg(count));
            return;
        }
        invokeBlockMethodWithTimeout(object, "setCurrentIndex", Q_ARG(int, currentIndex));
    };

    auto indexFromText = [&] {
        for (int i = 0; i < count; i++) {
            QString currentText;
            bool ok = QMetaObject::invokeMethod(object, "tabText", Qt::BlockingQueuedConnection,
                                                Q_RETURN_ARG(QString, currentText), Q_ARG(int, i));
            assert(ok == true);
            if (currentText == text) {
                return i;
            }
        }
        return -1;
    };

    switch (behavior) {
    case TextIndexBehavior::OnlyIndex: {
        indexHandler(index);
        break;
    }
    case TextIndexBehavior::OnlyText: {
        auto textIndex = indexFromText();
        if (textIndex == -1) {
            engine_->throwError(QStringLiteral("Tab with text '%1' does not exist").arg(text));
            break;
        }
        indexHandler(textIndex);
        break;
    }
    case TextIndexBehavior::TextIndex: {
        auto textIndex = indexFromText();
        if (textIndex == -1) {
            emit scriptWarning(QStringLiteral("'%1': tab with text '%2' does not exist, "
                                              "trying to use index '%3' instead")
                                   .arg(path)
                                   .arg(text)
                                   .arg(index));
            indexHandler(index);
        }
        else {
            assert(textIndex < count);
            indexHandler(textIndex);
        }
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}

void ScriptRunner::selectTabItem(const QString &path, int index) const noexcept
{
    selectTabItemTemplate(path, index, QString(), TextIndexBehavior::OnlyIndex);
}

void ScriptRunner::selectTabItem(const QString &path, const QString &text) const noexcept
{
    selectTabItemTemplate(path, -1, path, TextIndexBehavior::OnlyText);
}

void ScriptRunner::selectTabItem(const QString &path, const QString &text, int index) const noexcept
{
    selectTabItemTemplate(path, index, path, TextIndexBehavior::TextIndex);
}

void ScriptRunner::treeViewTemplate(const QString &path, const QList<int> &indexPath,
                                    bool isExpand) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    if (!object->inherits("QTreeView")) {
        engine_->throwError(QStringLiteral("Passed object is not a tree"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    auto *view = qobject_cast<QAbstractItemView *>(object);
    assert(view != nullptr);
    auto *model = view->model();

    if (model == nullptr) {
        engine_->throwError(QStringLiteral("Object model is not accessible"));
        return;
    }

    QModelIndex lastIndex;
    for (const auto &row : indexPath) {
        QModelIndex index;
        if (lastIndex.isValid()) {
            index = model->index(row, 0, lastIndex);
        }
        else {
            index = model->index(row, 0);
        }

        if (!index.isValid()) {
            engine_->throwError(QStringLiteral("Can't get accesible model index from path"));
            return;
        }

        if (isExpand) {
            invokeBlockMethodWithTimeout(object, "expand", Q_ARG(QModelIndex, index));
        }
        lastIndex = index;
    }

    if (!isExpand) {
        invokeBlockMethodWithTimeout(object, "collapse", Q_ARG(QModelIndex, lastIndex));
    }
}

void ScriptRunner::expandDelegate(const QString &path, const QList<int> &indexPath) const noexcept
{
    treeViewTemplate(path, indexPath, true);
}

void ScriptRunner::collapseDelegate(const QString &path, const QList<int> &indexPath) const noexcept
{
    treeViewTemplate(path, indexPath, false);
}

void ScriptRunner::undoCommand(const QString &path, int index) const noexcept
{
    emit scriptWarning(QStringLiteral("In this QtAda version function 'undoCommand' is unstable, "
                                      "so it is better to use 'delegateClick' function"));
}

void ScriptRunner::selectViewItem(const QString &path, int index) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    if (!object->inherits("QQuickPathView") && !object->inherits("QQuickSwipeView")) {
        engine_->throwError(QStringLiteral("Passed object is not a path or swipe view"));
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

    if (index < 0 || index >= count) {
        engine_->throwError(
            QStringLiteral("Index '%1' is out of range [0, %2)").arg(index).arg(count));
        return;
    }
    writePropertyInGuiThread(object, "currentIndex", index);
}

void ScriptRunner::actionTemplate(const QString &path, std::optional<bool> isChecked) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    if (!object->inherits("QAction")) {
        engine_->throwError(QStringLiteral("Passed object is not an action"));
        return;
    }
    if (!checkObjectAvailability(object, path, false)) {
        return;
    }

    if (!isChecked.has_value()) {
        invokeBlockMethodWithTimeout(object, "trigger");
        return;
    }

    if (!utils::getFromVariant<bool>(QQmlProperty::read(object, "checkable"))) {
        engine_->throwError(QStringLiteral("Action is not checkable"));
        return;
    }

    if (utils::getFromVariant<bool>(QQmlProperty::read(object, "checked")) == *isChecked) {
        emit scriptWarning(QStringLiteral("'%1': button already has state '%2'")
                               .arg(path)
                               .arg(*isChecked ? "true" : "false"));
    }
    else {
        invokeBlockMethodWithTimeout(object, "toggle");
    }
}

void ScriptRunner::triggerAction(const QString &path) const noexcept
{
    actionTemplate(path, std::nullopt);
}

void ScriptRunner::triggerAction(const QString &path, bool isChecked) const noexcept
{
    actionTemplate(path, isChecked);
}

void ScriptRunner::delegateTemplate(const QString &path, int index, bool isDouble) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    if (!object->inherits("QQuickItemView")) {
        engine_->throwError(QStringLiteral("Passed object is not an item view"));
        return;
    }
    if (!checkObjectAvailability(object, path, false)) {
        return;
    }

    const auto count = utils::getFromVariant<int>(QQmlProperty::read(object, "count"));
    if (count == 0) {
        engine_->throwError(QStringLiteral("Passed object has no delegates"));
        return;
    }

    if (index < 0 || index >= count) {
        engine_->throwError(
            QStringLiteral("Index '%1' is out of range [0, %2)").arg(index).arg(count));
        return;
    }

    QQuickItem *item = nullptr;
    bool ok = QMetaObject::invokeMethod(object, "itemAtIndex", Qt::BlockingQueuedConnection,
                                        Q_RETURN_ARG(QQuickItem *, item), Q_ARG(int, index));
    assert(ok == true);

    if (item == nullptr) {
        engine_->throwError(
            QStringLiteral("Delegate with index '%1' is not accessible").arg(index));
        return;
    }

    const auto height = utils::getFromVariant<double>(QQmlProperty::read(object, "height"));
    const auto width = utils::getFromVariant<double>(QQmlProperty::read(object, "width"));
    const auto pos = QPoint(width / 2, height / 2);

    if (isDouble) {
        postEvents(item, { simpleMouseEvent(QEvent::MouseButtonDblClick, pos) });
    }
    else {
        postEvents(item, { simpleMouseEvent(QEvent::MouseButtonPress, pos),
                           simpleMouseEvent(QEvent::MouseButtonRelease, pos) });
    }
}

void ScriptRunner::delegateClick(const QString &path, int index) const noexcept
{
    delegateTemplate(path, index, false);
}

void ScriptRunner::delegateDblClick(const QString &path, int index) const noexcept
{
    delegateTemplate(path, index, true);
}

void ScriptRunner::delegateTemplate(const QString &path, std::optional<QList<int>> indexPath,
                                    std::optional<std::pair<int, int>> index,
                                    bool isDouble) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    bool isTreeView = false;
    bool isUsualView = false;

    if (indexPath.has_value()) {
        assert(!index.has_value());
        isTreeView = object->inherits("QTreeView");
    }
    else if (index.has_value()) {
        assert(!indexPath.has_value());
        isUsualView = object->inherits("QAbstractItemView");
    }
    else {
        Q_UNREACHABLE();
    }

    if (!isTreeView && !isUsualView) {
        engine_->throwError(QStringLiteral("Passed object is not a %1")
                                .arg(indexPath.has_value() ? "tree" : "view"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    auto *view = qobject_cast<QAbstractItemView *>(object);
    assert(view != nullptr);
    auto *model = view->model();

    if (model == nullptr) {
        engine_->throwError(QStringLiteral("Object model is not accessible"));
        return;
    }

    QModelIndex lastIndex;
    if (isTreeView) {
        assert(!index.has_value());
        for (const auto &row : *indexPath) {
            QModelIndex index;
            if (lastIndex.isValid()) {
                index = model->index(row, 0, lastIndex);
            }
            else {
                index = model->index(row, 0);
            }

            if (!index.isValid()) {
                engine_->throwError(QStringLiteral("Can't get accesible model index from path"));
                return;
            }
            lastIndex = index;
        }
    }
    else if (isUsualView) {
        lastIndex = model->index(index->first, index->second);
    }
    else {
        Q_UNREACHABLE();
    }

    const auto rect = view->visualRect(lastIndex);
    if (rect.isEmpty() || rect.isNull()) {
        engine_->throwError(QStringLiteral("Delegate is not accessible"));
        return;
    }

    const auto pos = rect.center();
    auto *viewport = view->viewport();
    assert(viewport != nullptr);

    std::vector<QEvent *> events;
    events.push_back(simpleMouseEvent(QEvent::MouseButtonPress, pos));
    events.push_back(simpleMouseEvent(QEvent::MouseButtonRelease, pos));
    if (isDouble) {
        events.push_back(simpleMouseEvent(QEvent::MouseButtonDblClick, pos));
        events.push_back(simpleMouseEvent(QEvent::MouseButtonRelease, pos));
    }
    postEvents(viewport, events);
}

void ScriptRunner::delegateClick(const QString &path, QList<int> indexPath) const noexcept
{
    delegateTemplate(path, indexPath, std::nullopt, false);
}

void ScriptRunner::delegateDblClick(const QString &path, QList<int> indexPath) const noexcept
{
    delegateTemplate(path, indexPath, std::nullopt, true);
}

void ScriptRunner::delegateClick(const QString &path, int row, int column) const noexcept
{
    delegateTemplate(path, std::nullopt, std::make_pair(row, column), false);
}

void ScriptRunner::delegateDblClick(const QString &path, int row, int column) const noexcept
{
    delegateTemplate(path, std::nullopt, std::make_pair(row, column), true);
}

void ScriptRunner::setSelection(const QString &path, const QJSValue &selectionData) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    if (!object->inherits("QAbstractItemView")) {
        engine_->throwError(QStringLiteral("Passed object is not a view"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    auto *view = qobject_cast<QAbstractItemView *>(object);
    assert(view != nullptr);
    auto *model = view->model();
    if (model == nullptr) {
        engine_->throwError(QStringLiteral("Object model is not accessible"));
        return;
    }
    auto *selectionModel = view->selectionModel();
    if (selectionModel == nullptr) {
        engine_->throwError(QStringLiteral("Object selection model is not accessible"));
        return;
    }

    bool isOk = true;
    const auto rowCount = model->rowCount();
    const auto columnCount = model->columnCount();
    const auto data = parseSelectionData(selectionData, isOk, rowCount, columnCount, engine_);
    if (!isOk) {
        return;
    }

    QItemSelection selection;
    invokeBlockMethodWithTimeout(selectionModel, "clearSelection");
    //! TODO: слишком много дублирования кода, нужно будет переписать
    for (const auto &pair : data) {
        const auto &rawRow = pair.first;
        const auto &rawColumn = pair.second;
        const auto rawRowType = rawRow.type();
        const auto rawColumnType = rawColumn.type();

        if (rawRowType == QVariant::Bool) {
            if (rawColumnType == QVariant::Bool) {
                for (int r = 0; r < rowCount; r++) {
                    for (int c = 0; c < columnCount; c++) {
                        auto index = model->index(r, c);
                        selection.select(index, index);
                    }
                }
                break;
            }
            else {
                const auto cols = rawColumn.value<QList<int>>();
                assert(!cols.isEmpty());
                for (int r = 0; r < rowCount; r++) {
                    for (const auto &c : cols) {
                        auto index = model->index(r, c);
                        selection.select(index, index);
                    }
                }
            }
        }
        else if (rawRowType == QVariant::Int) {
            const auto row = rawRow.toInt();
            if (rawColumnType == QVariant::Bool) {
                for (int c = 0; c < columnCount; c++) {
                    auto index = model->index(row, c);
                    selection.select(index, index);
                }
            }
            else {
                const auto cols = rawColumn.value<QList<int>>();
                assert(!cols.isEmpty());
                for (const auto &c : cols) {
                    auto index = model->index(row, c);
                    selection.select(index, index);
                }
            }
        }
        else {
            Q_UNREACHABLE();
        }
    }
    invokeBlockMethodWithTimeout(
        selectionModel, "select", Q_ARG(QItemSelection, selection),
        Q_ARG(QItemSelectionModel::SelectionFlags, QItemSelectionModel::Select));
}

void ScriptRunner::clearSelection(const QString &path) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    if (!object->inherits("QAbstractItemView")) {
        engine_->throwError(QStringLiteral("Passed object is not a view"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    auto *view = qobject_cast<QAbstractItemView *>(object);
    assert(view != nullptr);
    auto *selectionModel = view->selectionModel();
    if (selectionModel == nullptr) {
        engine_->throwError(QStringLiteral("Object selection model is not accessible"));
        return;
    }
    invokeBlockMethodWithTimeout(selectionModel, "clearSelection");
}

void ScriptRunner::setText(const QString &path, const QString &text) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    //! TODO: Нужно тщательней проверить какие еще компоненты могут иметь "под копотом"
    //! QLineEdit, чтобы избежать ошибки "Passed object is not an text edit".
    const auto isWidgetComboBox = object->inherits("QComboBox");
    const auto isWidgetSpinBox = object->inherits("QAbstractSpinBox");
    const auto isQuickComboBox = object->inherits("QQuickComboBox");
    const auto isQuickSpinBox = object->inherits("QQuickSpinBox");
    if (isWidgetComboBox || isWidgetSpinBox || isQuickComboBox || isQuickSpinBox) {
        const auto isEditable
            = isWidgetSpinBox ? !utils::getFromVariant<bool>(QQmlProperty::read(object, "readOnly"))
                              : utils::getFromVariant<bool>(QQmlProperty::read(object, "editable"));
        if (!isEditable) {
            engine_->throwError(
                QStringLiteral("Passed %1 is not editable")
                    .arg(isWidgetComboBox || isQuickComboBox ? "ComboBox" : "SpinBox"));
            return;
        }

        if (isWidgetSpinBox) {
            if (object->inherits("QSpinBox") || object->inherits("QDoubleSpinBox")) {
                auto doubleValue
                    = utils::getFromVariant<double>(QQmlProperty::read(object, "value"));
                setValueTemplate(object, path, doubleValue);
            }
            else {
                setValueTemplate(object, path, text);
            }
        }
        else if (isWidgetComboBox) {
            invokeBlockMethodWithTimeout(object, "setEditText", Q_ARG(QString, text));
        }
        else if (isQuickSpinBox) {
            setValueTemplate(object, path, text);
        }
        else if (isQuickComboBox) {
            writePropertyInGuiThread(object, "editText", text);
        }
        else {
            Q_UNREACHABLE();
        }
        return;
    }

    const auto isQuickTextEdit
        = object->inherits("QQuickTextEdit") || object->inherits("QQuickTextInput");
    const auto isWidgetTextEdit = object->inherits("QTextEdit") || object->inherits("QLineEdit");
    const auto isWidgetPlainTextEdit = object->inherits("QPlainTextEdit");
    const auto isWidgetKeySeqEdit = object->inherits("QKeySequenceEdit");

    if (!isQuickTextEdit && !isWidgetTextEdit && !isWidgetPlainTextEdit && !isWidgetKeySeqEdit) {
        engine_->throwError(QStringLiteral("Passed object is not an text edit"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    if (isQuickTextEdit) {
        writePropertyInGuiThread(object, "text", text);
    }
    else if (isWidgetTextEdit) {
        invokeBlockMethodWithTimeout(object, "setText", Q_ARG(QString, text));
    }
    else if (isWidgetPlainTextEdit) {
        invokeBlockMethodWithTimeout(object, "setPlainText", Q_ARG(QString, text));
    }
    else if (isWidgetKeySeqEdit) {
        invokeBlockMethodWithTimeout(object, "setKeySequence",
                                     Q_ARG(QKeySequence, QKeySequence(text)));
    }
    else {
        Q_UNREACHABLE();
    }
}

void ScriptRunner::setTextTemplate(const QString &path, std::optional<QList<int>> indexPath,
                                   std::optional<std::pair<int, int>> index,
                                   const QString &text) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    bool isTreeView = false;
    bool isUsualView = false;

    if (indexPath.has_value()) {
        assert(!index.has_value());
        isTreeView = object->inherits("QTreeView");
    }
    else if (index.has_value()) {
        assert(!indexPath.has_value());
        isUsualView = object->inherits("QAbstractItemView");
    }
    else {
        Q_UNREACHABLE();
    }

    if (!isTreeView && !isUsualView) {
        engine_->throwError(QStringLiteral("Passed object is not a %1")
                                .arg(indexPath.has_value() ? "tree" : "view"));
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }

    auto *view = qobject_cast<QAbstractItemView *>(object);
    assert(view != nullptr);
    auto *model = view->model();

    if (model == nullptr) {
        engine_->throwError(QStringLiteral("Object model is not accessible"));
        return;
    }

    QModelIndex lastIndex;
    if (isTreeView) {
        assert(!index.has_value());
        for (const auto &row : *indexPath) {
            QModelIndex index;
            if (lastIndex.isValid()) {
                index = model->index(row, 0, lastIndex);
            }
            else {
                index = model->index(row, 0);
            }

            if (!index.isValid()) {
                engine_->throwError(QStringLiteral("Can't get accesible model index from path"));
                return;
            }
            lastIndex = index;
        }
    }
    else if (isUsualView) {
        lastIndex = model->index(index->first, index->second);
    }
    else {
        Q_UNREACHABLE();
    }
    invokeBlockMethodWithTimeout(model, "setData", Q_ARG(QModelIndex, lastIndex),
                                 Q_ARG(QVariant, QVariant(text)), Q_ARG(int, Qt::EditRole));
}

void ScriptRunner::setText(const QString &path, int row, int column,
                           const QString &text) const noexcept
{
    setTextTemplate(path, std::nullopt, std::make_pair(row, column), text);
}

void ScriptRunner::setText(const QString &path, QList<int> indexPath,
                           const QString &text) const noexcept
{
    setTextTemplate(path, indexPath, std::nullopt, text);
}

void ScriptRunner::closeDialog(const QString &path) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }
    if (!object->inherits("QDialog")) {
        engine_->throwError(QStringLiteral("Passed object is not a dialog"));
        return;
    }
    postEvents(object, { new QCloseEvent() });
}

void ScriptRunner::closeWindow(const QString &path) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }

    const auto isWidgetWindow = object->inherits("QMainWindow");
    const auto isQuickWindow = object->inherits("QQuickWindow");
    if (!isWidgetWindow && !isQuickWindow) {
        engine_->throwError(QStringLiteral("Passed object is not a window"));
        return;
    }

    if (isWidgetWindow) {
        //! TODO: Почему-то именно для QMainWindow не работает
        //! QGuiApplication::postEvent(object, new QCloseEvent());
        invokeBlockMethodWithTimeout(object, "close");
    }
    else if (isQuickWindow) {
        postEvents(object, { new QCloseEvent() });
    }
}

void ScriptRunner::keyEvent(const QString &path, const QString &keyText) const noexcept
{
    auto *object = findObjectByPath(path);
    if (object == nullptr) {
        return;
    }
    if (!checkObjectAvailability(object, path)) {
        return;
    }
    //! TODO: Пока непонятно, что делать с keyEvent и нужен ли он вообще. Сейчас он
    //! точно не важен, поэтому сделал "тестовый" вариант, но учитывая, что из-за
    //! "модификаторов" куча других действий могут потребовать правок (с учетом
    //! модификаторов обычные нажатия могут приводить к другим результатам).
    emit scriptWarning(QStringLiteral("In this QtAda version function 'keyEvent' is unstable, "
                                      "so be attentive to use it"));
    QKeySequence keySequence(keyText);
    const auto key = keySequence[0];
    const auto modifiers = Qt::KeyboardModifiers(keySequence[0] & Qt::KeyboardModifierMask);
    postEvents(object, { new QKeyEvent(QEvent::KeyPress, key, modifiers, keyText),
                         new QKeyEvent(QEvent::KeyRelease, key, modifiers, keyText) });
}

void ScriptRunner::wheelEvent(const QString &path, int dx, int dy) const noexcept
{
    //! TODO: Не получается повторить QWheelEvent. Причем пробовал передавать
    //! вообще все аргументы, которые касаются события - все равно события не
    //! происходило. Сейчас оно далеко не в приоритете, поэтому откладываем.
    emit scriptWarning(QStringLiteral("In this QtAda version function 'wheelEvent' is unstable, "
                                      "so it is better to use 'mouseClick' if it is possible"));
}
} // namespace QtAda::core
