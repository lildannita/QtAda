#include "PreloadInjector.hpp"

#include "LauncherUtils.hpp"

namespace QtAda::launcher::injector {
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
    /*
     * Эта переменная указывает динамической библиотеке, что нам больше не нужно
     * иметь LD_PRELOAD в окружении, чтобы не использовать Hooks для дочерних
     * процессов, которые может запустить тестируемое приложение
     */
    _env.insert(QStringLiteral("QTADA_NEED_TO_UNSET_PRELOAD"), QStringLiteral("1"));

    //! TODO: remove, когда будет готов GUI QtAda
    _env.insert(QStringLiteral("QTADA_GENERATION_SETTINGS"),
                QStringLiteral(
                    "textIndexBehavoir=0;duplicateMouseEvent=0;scriptPath=/files/trash/qtada.js"));

    return injectAndLaunch(launchArgs, _env);
}
} // namespace QtAda::launcher::injector
