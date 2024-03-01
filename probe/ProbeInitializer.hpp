#pragma once

#include <QObject>

namespace probe {
class ProbeInitializer final : public QObject {
    Q_OBJECT
public:
    ProbeInitializer() noexcept;
private slots:
    void initProbe() noexcept;
};
}
