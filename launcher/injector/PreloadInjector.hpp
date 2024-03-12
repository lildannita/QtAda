#pragma once

#include "ProcessInjector.hpp"

namespace QtAda::launcher::injector {
class PreloadInjector final : public ProcessInjector {
    Q_OBJECT
public:
    bool launch(const QStringList &launchArgs, const QString &probeDllPath,
                const QProcessEnvironment &env) noexcept override;

private:
};
} // namespace QtAda::launcher::injector
