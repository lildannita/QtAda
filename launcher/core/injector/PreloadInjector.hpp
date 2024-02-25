#pragma once

#include "ProcessInjector.hpp"

namespace launcher::injector {
class PreloadInjector final : public ProcessInjector {
    Q_OBJECT
public:
    bool launch(const QStringList &launchArgs, const QString &probeDllPath,
                const QProcessEnvironment &env) noexcept override;

private:
};
} // namespace launcher::injector
