#pragma once

#include <QObject>

namespace QtAda::core {
class MetaObjectHandler : public QObject {
    Q_OBJECT
public:
    explicit MetaObjectHandler(QObject *parent = nullptr) noexcept;
    ~MetaObjectHandler() noexcept;
};
}
