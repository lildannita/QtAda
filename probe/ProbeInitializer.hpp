#pragma once

#include <QObject>

#include "GenerationSettings.hpp"

namespace QtAda::probe {
class ProbeInitializer final : public QObject {
    Q_OBJECT
public:
    ProbeInitializer() noexcept;
private slots:
    void initProbe() noexcept;

private:
    core::GenerationSettings readSettings() const noexcept;
};
} // namespace QtAda::probe
