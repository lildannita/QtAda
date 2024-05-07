#pragma once

#include <QObject>

#include "Settings.hpp"

namespace QtAda::core {
class ScriptRunner final : public QObject {
    Q_OBJECT
public:
    ScriptRunner(const RunSettings &settings, QObject *parent = nullptr) noexcept;

    int exitCode() const noexcept
    {
        return exitCode_;
    }

signals:
    void scriptError(const QString &msg);
    //! TODO: пока не понятно, нужен ли этот сигнал,
    //! но скорее всего надо будет убрать
    void scriptLog(const QString &msg);

public slots:
    void startScript() noexcept;

    void registerObjectCreated(QObject *obj) noexcept;
    void registerObjectDestroyed(QObject *obj) noexcept;
    void registerObjectReparented(QObject *obj) noexcept;

private:
    /*
     * Приходится использовать отдельную переменную, так как
     * хоть QThread и имеет метод `exit(int exitCode)`, но
     * получить этот код возможности нет.
     * (-1) - работа потока еще не завершена
     * (1) - работа потока завершена с ошибкой
     * (0) - работа потока завершена успешно
     */
    int exitCode_ = -1;

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
