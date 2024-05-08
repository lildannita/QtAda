#pragma once

#include <QObject>

#include "Settings.hpp"

namespace QtAda::core {
class ScriptRunner final : public QObject {
    Q_OBJECT
public:
    ScriptRunner(const RunSettings &settings, QObject *parent = nullptr) noexcept;

    Q_INVOKABLE void mouseClick(const QString &path) noexcept;

signals:
    void scriptError(const QString &msg);
    //! TODO: пока не понятно, нужен ли этот сигнал,
    //! но скорее всего надо будет убрать
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

    QObject *findObjectByPath(const QString &path) noexcept
    {
        //! TODO: Здесь и будет "засыпать" текущий поток
        return nullptr;
    }

    void finishThread(bool isOk) noexcept;
};
} // namespace QtAda::core
