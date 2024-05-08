#pragma once

#include <QObject>

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
                            const QString &value) noexcept;
    Q_INVOKABLE void mouseClick(const QString &path, const QString &mouseButtonStr, int x,
                                int y) noexcept;

signals:
    void scriptError(const QString &msg);
    void scriptLog(const QString &msg);

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

    QObject *findObjectByPath(const QString &path) noexcept;

    void finishThread(bool isOk) noexcept;
};
} // namespace QtAda::core
