#include "PreloadInjector.hpp"

#include "../LauncherUtils.hpp"

namespace launcher::injector {
bool PreloadInjector::launch(const QStringList &launchArgs, const QString &probeDllPath,
                             const QProcessEnvironment &env) noexcept
{
    assert(!launchArgs.isEmpty());
    assert(!probeDllPath.isEmpty());

    QStringList preloadLibs;
    const auto exePath = launchArgs.constFirst();
    // AddressSanitizer необходимо загрузить перед всеми остальными библиотеками
    for (const auto &lib : utils::getDependenciesForExecutable(exePath)) {
        if (lib.contains("libasan.so") || lib.contains("libclang_rt.asan")) {
            preloadLibs.push_back(QString::fromLocal8Bit(lib));
            break;
        }
    }
    preloadLibs.push_back(probeDllPath);

    QProcessEnvironment _env(env);
    _env.insert(QStringLiteral("LD_PRELOAD"), preloadLibs.join(QLatin1String(":")));
    return injectAndLaunch(launchArgs, env);
}
} // namespace launcher::injector
