#pragma once

#include <QObject>
#include <QEvent>

#include "Settings.hpp"

QT_BEGIN_NAMESPACE
class QJSEngine;
class QJSValue;
QT_END_NAMESPACE

namespace QtAda::core {
class ScriptRunner final : public QObject {
    Q_OBJECT
public:
    ScriptRunner(const RunSettings &settings, QObject *parent = nullptr) noexcept;

    Q_INVOKABLE void verify(const QString &path, const QString &property,
                            const QString &value) const noexcept;
    Q_INVOKABLE void sleep(int sec);
    Q_INVOKABLE void msleep(int msec);
    Q_INVOKABLE void usleep(int usec);
    Q_INVOKABLE void mouseClick(const QString &path, const QString &mouseButtonStr, int x,
                                int y) const noexcept;
    Q_INVOKABLE void mouseDblClick(const QString &path, const QString &mouseButtonStr, int x,
                                   int y) const noexcept;
    Q_INVOKABLE void keyEvent(const QString &path, const QString &keyText) const noexcept;
    Q_INVOKABLE void wheelEvent(const QString &path, int dx, int dy) const noexcept;
    Q_INVOKABLE void buttonClick(const QString &path) const noexcept;
    Q_INVOKABLE void buttonToggle(const QString &path) const noexcept;
    Q_INVOKABLE void buttonDblClick(const QString &path) const noexcept;
    Q_INVOKABLE void buttonPress(const QString &path) const noexcept;
    Q_INVOKABLE void mouseAreaClick(const QString &path) const noexcept;
    Q_INVOKABLE void mouseAreaDblClick(const QString &path) const noexcept;
    Q_INVOKABLE void mouseAreaPress(const QString &path) const noexcept;
    Q_INVOKABLE void checkButton(const QString &path, bool isChecked) const noexcept;
    Q_INVOKABLE void selectItem(const QString &path, int index) const noexcept;
    Q_INVOKABLE void selectItem(const QString &path, const QString &text) const noexcept;
    Q_INVOKABLE void selectItem(const QString &path, const QString &text, int index) const noexcept;
    Q_INVOKABLE void setValue(const QString &path, double value) const noexcept;
    Q_INVOKABLE void setValue(const QString &path, double leftValue,
                              double rightValue) const noexcept;
    Q_INVOKABLE void setValue(const QString &path, const QString &value) const noexcept;
    Q_INVOKABLE void changeValue(const QString &path, const QString &type) const noexcept;
    Q_INVOKABLE void setDelayProgress(const QString &path, double delay) const noexcept;
    Q_INVOKABLE void selectTabItem(const QString &path, int index) const noexcept;
    Q_INVOKABLE void selectTabItem(const QString &path, const QString &text) const noexcept;
    Q_INVOKABLE void selectTabItem(const QString &path, const QString &text,
                                   int index) const noexcept;
    Q_INVOKABLE void expandDelegate(const QString &path,
                                    const QList<int> &indexPath) const noexcept;
    Q_INVOKABLE void collapseDelegate(const QString &path,
                                      const QList<int> &indexPath) const noexcept;
    Q_INVOKABLE void undoCommand(const QString &path, int index) const noexcept;
    Q_INVOKABLE void selectViewItem(const QString &path, int index) const noexcept;
    Q_INVOKABLE void triggerAction(const QString &path) const noexcept;
    Q_INVOKABLE void triggerAction(const QString &path, bool isChecked) const noexcept;
    Q_INVOKABLE void delegateClick(const QString &path, int index) const noexcept;
    Q_INVOKABLE void delegateDblClick(const QString &path, int index) const noexcept;
    Q_INVOKABLE void delegateClick(const QString &path, QList<int> indexPath) const noexcept;
    Q_INVOKABLE void delegateDblClick(const QString &path, QList<int> indexPath) const noexcept;
    Q_INVOKABLE void delegateClick(const QString &path, int row, int column) const noexcept;
    Q_INVOKABLE void delegateDblClick(const QString &path, int row, int column) const noexcept;
    Q_INVOKABLE void setSelection(const QString &path,
                                  const QJSValue &selectionData) const noexcept;
    Q_INVOKABLE void clearSelection(const QString &path) const noexcept;
    Q_INVOKABLE void setText(const QString &path, const QString &text) const noexcept;
    Q_INVOKABLE void setText(const QString &path, int row, int column,
                             const QString &text) const noexcept;
    Q_INVOKABLE void setText(const QString &path, QList<int> indexPath,
                             const QString &text) const noexcept;
    Q_INVOKABLE void closeDialog(const QString &path) const noexcept;
    Q_INVOKABLE void closeWindow(const QString &path) const noexcept;

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

    const RunSettings runSettings_;
    QJSEngine *engine_ = nullptr;

    void finishThread(bool isOk) noexcept;

    void writePropertyInGuiThread(QObject *object, const QString &propertyName,
                                  const QVariant &value) const noexcept;
    void invokeBlockMethodWithTimeout(QObject *object, const char *method,
                                      QGenericArgument val0 = QGenericArgument(nullptr),
                                      QGenericArgument val1 = QGenericArgument(),
                                      QGenericArgument val2 = QGenericArgument()) const noexcept;
    void postEvents(QObject *object, std::vector<QEvent *> events) const noexcept;

    QObject *findObjectByPath(const QString &path) const noexcept;
    bool checkObjectAvailability(const QObject *object, const QString &path,
                                 bool shouldBeVisible = true) const noexcept;

    void mouseClickTemplate(const QString &path, const QString &mouseButtonStr, int x, int y,
                            bool isDouble) const noexcept;
    void mouseAreaEventTemplate(const QString &path,
                                const std::vector<QEvent::Type> eventTypes) const noexcept;
    void selectItemTemplate(const QString &path, int index, const QString &text,
                            TextIndexBehavior behavior) const noexcept;
    void selectTabItemTemplate(const QString &path, int index, const QString &text,
                               TextIndexBehavior behavior) const noexcept;
    void treeViewTemplate(const QString &path, const QList<int> &indexPath,
                          bool isExpand) const noexcept;
    void actionTemplate(const QString &path, std::optional<bool> isChecked) const noexcept;
    void delegateTemplate(const QString &path, int index, bool isDouble) const noexcept;
    void delegateTemplate(const QString &path, std::optional<QList<int>> indexPath,
                          std::optional<std::pair<int, int>> index, bool isDouble) const noexcept;
    void setTextTemplate(const QString &path, std::optional<QList<int>> indexPath,
                         std::optional<std::pair<int, int>> index,
                         const QString &text) const noexcept;
};
} // namespace QtAda::core
