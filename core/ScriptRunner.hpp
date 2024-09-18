#pragma once

#include <QObject>
#include <QEvent>

#include "ScreenshotManager.hpp"
#include "Settings.hpp"
#include "Common.hpp"

QT_BEGIN_NAMESPACE
class QJSEngine;
class QJSValue;
QT_END_NAMESPACE

#define GENERATE_ACTION_FUNCTION(funcName)                                                         \
    Q_INVOKABLE void funcName(QObject *object) const noexcept                                      \
    {                                                                                              \
        if (!checkObjectPointer(object)) {                                                         \
            return;                                                                                \
        }                                                                                          \
        const auto canBeVisible = canObjectBeVisible(object);                                      \
        if (!objectHasAvailabilityProperties(object, canBeVisible)                                 \
            || !checkObjectAvailability(object, canBeVisible, true)) {                             \
            return;                                                                                \
        }                                                                                          \
        const auto functionName = QStringLiteral(#funcName);                                       \
        takeScreenshot(object, functionName);                                                      \
        do_##funcName(object);                                                                     \
    }                                                                                              \
    Q_INVOKABLE void funcName(const QString &path) const noexcept                                  \
    {                                                                                              \
        auto *object = waitAndGetObject(path, std::nullopt, true);                                 \
        if (object == nullptr) {                                                                   \
            return;                                                                                \
        }                                                                                          \
        const auto functionName = QStringLiteral(#funcName);                                       \
        takeScreenshot(object, functionName);                                                      \
        do_##funcName(object);                                                                     \
    }
#define GROUP(...) __VA_ARGS__
#define GENERATE_ACTION_FUNCTION_WITH_ARGS(funcName, declaration, values)                          \
    Q_INVOKABLE void funcName(QObject *object, declaration) const noexcept                         \
    {                                                                                              \
        if (!checkObjectPointer(object)) {                                                         \
            return;                                                                                \
        }                                                                                          \
        const auto canBeVisible = canObjectBeVisible(object);                                      \
        if (!objectHasAvailabilityProperties(object, canBeVisible)                                 \
            || !checkObjectAvailability(object, canBeVisible, true)) {                             \
            return;                                                                                \
        }                                                                                          \
        const auto functionName = QStringLiteral(#funcName);                                       \
        takeScreenshot(object, functionName);                                                      \
        do_##funcName(object, values);                                                             \
    }                                                                                              \
    Q_INVOKABLE void funcName(const QString &path, declaration) const noexcept                     \
    {                                                                                              \
        auto *object = waitAndGetObject(path, std::nullopt, true);                                 \
        if (object == nullptr) {                                                                   \
            return;                                                                                \
        }                                                                                          \
        const auto functionName = QStringLiteral(#funcName);                                       \
        takeScreenshot(object, functionName);                                                      \
        do_##funcName(object, values);                                                             \
    }

namespace QtAda::core {
class ScreenshotManager;
class ScriptRunner final : public QObject {
    Q_OBJECT
public:
    ScriptRunner(const RunSettings &settings, QObject *parent = nullptr) noexcept;
    void handleApplicationClosing() noexcept
    {
        pathToObject_.clear();
        objectToPath_.clear();
    }

    // ************** Script API **************
    Q_INVOKABLE void sleep(int sec);
    Q_INVOKABLE void msleep(int msec);
    Q_INVOKABLE void usleep(int usec);

    Q_INVOKABLE void setDefaultWaitTimeout(int sec) noexcept;
    Q_INVOKABLE void msetDefaultWaitTimeout(int msec) noexcept;
    Q_INVOKABLE void setDefaultInvokeTimeout(int sec) noexcept;
    Q_INVOKABLE void msetDefaultInvokeTimeout(int msec) noexcept;
    Q_INVOKABLE void setDefaultVerifyTimeout(int sec) noexcept;
    Q_INVOKABLE void msetDefaultVerifyTimeout(int msec) noexcept;

    //! TODO: реализовать
    Q_INVOKABLE QObject *startApplication(const QString &id) noexcept
    {
        Q_UNUSED(id)
        return nullptr;
    }
    Q_INVOKABLE QObject *startApplication(const QString &id, int timeout) noexcept
    {
        Q_UNUSED(timeout)
        return startApplication(id);
    }
    Q_INVOKABLE void setCurrentApplication(QObject *application) noexcept
    {
        Q_UNUSED(application)
    }

    // ************** Tools API **************
    Q_INVOKABLE QString getEnv(const QString &variable) const noexcept
    {
        return qgetenv(variable.toUtf8().constData());
    }

    // ************** Objects API **************
    Q_INVOKABLE QObject *getObject(const QString &path) const noexcept;
    Q_INVOKABLE QObject *waitFor(const QString &path, int sec) const noexcept;
    Q_INVOKABLE QObject *mwaitFor(const QString &path, int msec) const noexcept;
    Q_INVOKABLE QObject *waitForCreation(const QString &path, int sec) const noexcept;
    Q_INVOKABLE QObject *mwaitForCreation(const QString &path, int msec) const noexcept;

    // ************** Test API **************
    GENERATE_ACTION_FUNCTION_WITH_ARGS(verify, GROUP(const QString &property, const QString &value),
                                       GROUP(property, value, std::nullopt))
    GENERATE_ACTION_FUNCTION_WITH_ARGS(waitForVerify,
                                       GROUP(const QString &property, const QString &value,
                                             int sec),
                                       GROUP(property, value, sec))
    GENERATE_ACTION_FUNCTION_WITH_ARGS(mwaitForVerify,
                                       GROUP(const QString &property, const QString &value,
                                             int msec),
                                       GROUP(property, value, msec))

    // ************** Actions API **************
    GENERATE_ACTION_FUNCTION_WITH_ARGS(mouseClick,
                                       GROUP(const QString &mouseButtonStr, int x, int y),
                                       GROUP(mouseButtonStr, x, y))
    GENERATE_ACTION_FUNCTION_WITH_ARGS(mouseDblClick,
                                       GROUP(const QString &mouseButtonStr, int x, int y),
                                       GROUP(mouseButtonStr, x, y))
    GENERATE_ACTION_FUNCTION_WITH_ARGS(keyEvent, const QString &keyText, keyText)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(wheelEvent, GROUP(int dx, int dy), GROUP(dx, dy))

    GENERATE_ACTION_FUNCTION(buttonClick)
    GENERATE_ACTION_FUNCTION(buttonToggle)
    GENERATE_ACTION_FUNCTION(buttonDblClick)
    GENERATE_ACTION_FUNCTION(buttonPress)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(buttonCheck, bool isChecked, isChecked)
    GENERATE_ACTION_FUNCTION(mouseAreaClick)
    GENERATE_ACTION_FUNCTION(mouseAreaDblClick)
    GENERATE_ACTION_FUNCTION(mouseAreaPress)

    GENERATE_ACTION_FUNCTION_WITH_ARGS(selectItem, int index, index)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(selectItem, const QString &text, text)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(selectItem, GROUP(const QString &text, int index),
                                       GROUP(text, index))
    GENERATE_ACTION_FUNCTION_WITH_ARGS(selectTabItem, int index, index)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(selectTabItem, const QString &text, text)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(selectTabItem, GROUP(const QString &text, int index),
                                       GROUP(text, index))

    GENERATE_ACTION_FUNCTION_WITH_ARGS(setValue, double value, value)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(setValue, GROUP(double leftValue, double rightValue),
                                       GROUP(leftValue, rightValue))
    GENERATE_ACTION_FUNCTION_WITH_ARGS(setValue, const QString &value, value)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(changeValue, const QString &type, type)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(setDelayProgress, double delay, delay)

    GENERATE_ACTION_FUNCTION_WITH_ARGS(expandDelegate, const QList<int> &indexPath, indexPath)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(collapseDelegate, const QList<int> &indexPath, indexPath)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(selectViewItem, int index, index)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(undoCommand, int index, index)
    GENERATE_ACTION_FUNCTION(triggerAction)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(triggerAction, bool isChecked, isChecked)

    GENERATE_ACTION_FUNCTION_WITH_ARGS(delegateClick, int index, index)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(delegateDblClick, int index, index)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(delegateClick, QList<int> indexPath, indexPath)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(delegateDblClick, QList<int> indexPath, indexPath)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(delegateClick, GROUP(int row, int column),
                                       GROUP(row, column))
    GENERATE_ACTION_FUNCTION_WITH_ARGS(delegateDblClick, GROUP(int row, int column),
                                       GROUP(row, column))

    GENERATE_ACTION_FUNCTION_WITH_ARGS(setSelection, const QJSValue &selectionData, selectionData)
    GENERATE_ACTION_FUNCTION(clearSelection)

    GENERATE_ACTION_FUNCTION_WITH_ARGS(setText, const QString &text, text)
    GENERATE_ACTION_FUNCTION_WITH_ARGS(setText, GROUP(int row, int column, const QString &text),
                                       GROUP(row, column, text))
    GENERATE_ACTION_FUNCTION_WITH_ARGS(setText, GROUP(QList<int> indexPath, const QString &text),
                                       GROUP(indexPath, text))

    GENERATE_ACTION_FUNCTION(closeDialog)
    GENERATE_ACTION_FUNCTION(closeWindow)

signals:
    void scriptError(const QString &msg) const;
    void scriptWarning(const QString &msg) const;
    void scriptLog(const QString &msg) const;

    void aboutToClose(int exitCode);

public slots:
    void startScript() noexcept;

    void registerObjectCreated(QObject *obj) noexcept;
    void registerObjectDestroyed(QObject *obj) noexcept;
    void registerObjectReparented(QObject *obj) noexcept;

private:
    std::map<const QString, QObject *> pathToObject_;
    std::map<const QObject *, QString> objectToPath_;
    std::map<const QString, QString> pathToId_;

    const RunSettings runSettings_;
    QJSEngine *engine_ = nullptr;

    std::unique_ptr<ScreenshotManager> screenshotManager_;

    void finishThread(bool isOk) noexcept
    {
        emit aboutToClose(isOk ? 0 : 1);
    }

    void writePropertyInGuiThread(QObject *object, const QString &propertyName,
                                  const QVariant &value) const noexcept;
    void invokeBlockMethodWithTimeout(QObject *object, const char *method,
                                      QGenericArgument val0 = QGenericArgument(nullptr),
                                      QGenericArgument val1 = QGenericArgument(),
                                      QGenericArgument val2 = QGenericArgument()) const noexcept;
    void postEvents(QObject *object, std::vector<QEvent *> events) const noexcept;

    QString tryToGetIdFromPath(const QString &path) const noexcept
    {
        if (pathToId_.count(path) > 0) {
            return pathToId_.at(path);
        }
        return path;
    }

    void takeScreenshot(const QObject *object, const QString &funcName) const noexcept
    {
        if (screenshotManager_ != nullptr) {
            screenshotManager_->newScreenshot(object, funcName);
        }
    }
    void takeErrorScreenshot(int lineNumber) const noexcept
    {
        if (screenshotManager_ != nullptr) {
            screenshotManager_->screenshotAllWindowsByError(lineNumber);
        }
    }
    void clearScreenshots() const noexcept
    {
        if (screenshotManager_ != nullptr) {
            screenshotManager_->clearScreenshots();
        }
    }

    // ************** Script API **************
    int waitTimeout_ = DEFAULT_SCRIPT_TIMEOUT_MS;
    int invokeTimeout_ = DEFAULT_SCRIPT_TIMEOUT_MS;
    int verifyTimeout_ = DEFAULT_SCRIPT_TIMEOUT_MS;
    bool checkTimeoutValue(int msec) const noexcept;

    // ************** Objects API **************
    bool checkObjectPointer(const QObject *object) const noexcept;
    bool canObjectBeVisible(const QObject *object) const noexcept;
    bool objectHasAvailabilityProperties(const QObject *object, bool canBeVisible) const noexcept;
    bool checkObjectAvailability(const QObject *object, bool canBeVisible,
                                 bool isCritical) const noexcept;
    QObject *waitAndGetObject(const QString &path, std::optional<int> msec,
                              bool waitForAccessibility) const noexcept;

    // ************** Test API **************
    void do_verify(QObject *object, const QString &property, const QString &value,
                   std::optional<int> msec) const noexcept;
    void do_waitForVerify(QObject *object, const QString &property, const QString &value,
                          int sec) const noexcept;
    void do_mwaitForVerify(QObject *object, const QString &property, const QString &value,
                           int msec) const noexcept;

    //! TODO: избавиться
    QObject *findObjectByPath(const QString &path) const noexcept;
    bool checkObjectAvailability(const QObject *object, const QString &path,
                                 bool shouldBeVisible = true) const noexcept;

    // ************** Actions API **************
    void mouseClickTemplate(QObject *object, const QString &mouseButtonStr, int x, int y,
                            bool isDouble) const noexcept;
    void do_mouseClick(QObject *object, const QString &mouseButtonStr, int x, int y) const noexcept;
    void do_mouseDblClick(QObject *object, const QString &mouseButtonStr, int x,
                          int y) const noexcept;
    void do_keyEvent(QObject *object, const QString &keyText) const noexcept;
    void do_wheelEvent(QObject *object, int dx, int dy) const noexcept;

    void do_buttonClick(QObject *object) const noexcept;
    void do_buttonToggle(QObject *object) const noexcept;
    void do_buttonDblClick(QObject *object) const noexcept;
    void do_buttonPress(QObject *object) const noexcept;
    void do_buttonCheck(QObject *object, bool isChecked) const noexcept;
    void mouseAreaEventTemplate(QObject *object,
                                const std::vector<QEvent::Type> eventTypes) const noexcept;
    void do_mouseAreaClick(QObject *object) const noexcept;
    void do_mouseAreaDblClick(QObject *object) const noexcept;
    void do_mouseAreaPress(QObject *object) const noexcept;

    void selectItemTemplate(QObject *object, int index, const QString &text,
                            TextIndexBehavior behavior) const noexcept;
    void do_selectItem(QObject *object, int index) const noexcept;
    void do_selectItem(QObject *object, const QString &text) const noexcept;
    void do_selectItem(QObject *object, const QString &text, int index) const noexcept;
    void selectTabItemTemplate(QObject *object, int index, const QString &text,
                               TextIndexBehavior behavior) const noexcept;
    void do_selectTabItem(QObject *object, int index) const noexcept;
    void do_selectTabItem(QObject *object, const QString &text) const noexcept;
    void do_selectTabItem(QObject *object, const QString &text, int index) const noexcept;

    void setValueIntoQmlSpinBox(QObject *object, const QString &value) const noexcept;
    void do_setValue(QObject *object, double value) const noexcept;
    void do_setValue(QObject *object, const QString &value) const noexcept;
    void do_setValue(QObject *object, double leftValue, double rightValue) const noexcept;
    void do_changeValue(QObject *object, const QString &type) const noexcept;
    void do_setDelayProgress(QObject *object, double delay) const noexcept;

    void treeViewTemplate(QObject *object, const QList<int> &indexPath,
                          bool isExpand) const noexcept;
    void do_expandDelegate(QObject *object, const QList<int> &indexPath) const noexcept;
    void do_collapseDelegate(QObject *object, const QList<int> &indexPath) const noexcept;
    void do_selectViewItem(QObject *object, int index) const noexcept;
    void do_undoCommand(QObject *object, int index) const noexcept;

    void actionTemplate(QObject *object, std::optional<bool> isChecked) const noexcept;
    void do_triggerAction(QObject *object) const noexcept;
    void do_triggerAction(QObject *object, bool isChecked) const noexcept;

    void delegateTemplate(QObject *object, int index, bool isDouble) const noexcept;
    void do_delegateClick(QObject *object, int index) const noexcept;
    void do_delegateDblClick(QObject *object, int index) const noexcept;
    void delegateTemplate(QObject *object, std::optional<QList<int>> indexPath,
                          std::optional<std::pair<int, int>> index, bool isDouble) const noexcept;
    void do_delegateClick(QObject *object, QList<int> indexPath) const noexcept;
    void do_delegateDblClick(QObject *object, QList<int> indexPath) const noexcept;
    void do_delegateClick(QObject *object, int row, int column) const noexcept;
    void do_delegateDblClick(QObject *object, int row, int column) const noexcept;

    void do_setSelection(QObject *object, const QJSValue &selectionData) const noexcept;
    void do_clearSelection(QObject *object) const noexcept;

    void do_setText(QObject *object, const QString &text) const noexcept;
    void setTextTemplate(QObject *object, std::optional<QList<int>> indexPath,
                         std::optional<std::pair<int, int>> index,
                         const QString &text) const noexcept;
    void do_setText(QObject *object, int row, int column, const QString &text) const noexcept;
    void do_setText(QObject *object, QList<int> indexPath, const QString &text) const noexcept;

    void do_closeWindow(QObject *object) const noexcept;
    void do_closeDialog(QObject *object) const noexcept;
};
} // namespace QtAda::core
