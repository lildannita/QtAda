#pragma once

#include <QObject>

namespace probe {
class ProbeInitializer final : public QObject {
    Q_OBJECT
public:
    ProbeInitializer() noexcept;
    void initProbe() noexcept;
};
}
