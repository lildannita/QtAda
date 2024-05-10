#pragma once

#include <QObject>
#include <QEvent>

#include "Settings.hpp"

QT_BEGIN_NAMESPACE
class QJSEngine;
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
    Q_INVOKABLE void buttonClick(const QString &path) const noexcept;
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

    bool writePropertyInGuiThread(QObject *object, const QString &propertyName,
                                  const QVariant &value) const noexcept;

    QObject *findObjectByPath(const QString &path) const noexcept;
    bool checkObjectAvailability(const QObject *object, const QString &path) const noexcept;
    void mouseAreaEventTemplate(const QString &path,
                                const std::vector<QEvent::Type> events) const noexcept;
    void selectItemTemplate(const QString &path, int index, const QString &text,
                            TextIndexBehavior behavior) const noexcept;
    void selectTabItemTemplate(const QString &path, int index, const QString &text,
                               TextIndexBehavior behavior) const noexcept;
    void treeViewTemplate(const QString &path, const QList<int> &indexPath,
                          bool isExpand) const noexcept;

    void finishThread(bool isOk) noexcept;
};
} // namespace QtAda::core
