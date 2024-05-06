#pragma once

#include <QObject>

#include "Settings.hpp"

namespace QtAda::core {
class ScriptRunner : public QObject {
    Q_OBJECT
public:
    ScriptRunner(const RunSettings &settings, QObject *parent = nullptr) noexcept;

public slots:
    void registerObjectCreated(QObject *obj) noexcept;
    void registerObjectDestroyed(QObject *obj) noexcept;
    void registerObjectReparented(QObject *obj) noexcept;

private:
    std::map<const QString, QObject *> pathToObject_;
    std::map<const QObject *, QString> objectToPath_;

    const RunSettings runSettings_;
};
} // namespace QtAda::core
