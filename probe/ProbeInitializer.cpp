#include "ProbeInitializer.hpp"

#include "Probe.hpp"

#include <QCoreApplication>
#include <QThread>
#include <QFileInfo>

namespace QtAda::probe {
using namespace core;
ProbeInitializer::ProbeInitializer() noexcept
{
    moveToThread(QCoreApplication::instance()->thread());
    QMetaObject::invokeMethod(this, "initProbe", Qt::QueuedConnection);

    if (qgetenv("QTADA_NEED_TO_UNSET_PRELOAD") == "1")
        qputenv("LD_PRELOAD", "");
}

void ProbeInitializer::initProbe() noexcept
{
    if (!qApp) {
        deleteLater();
        return;
    }

    assert(QThread::currentThread() == qApp->thread());

    Probe::initProbe(readSettings());
    assert(Probe::initialized());

    deleteLater();
}

GenerationSettings ProbeInitializer::readSettings() const noexcept
{
    //! TODO: remove, когда будет готов GUI QtAda
    qputenv("QTADA_GENERATION_SETTINGS",
            "textIndexBehavoir=0;duplicateMouseEvent=0;scriptPath=/files/trash/qtada.js");

    const auto envValue = qgetenv("QTADA_GENERATION_SETTINGS");
    const auto settings = QString(envValue).split(';');
    assert(settings.size() == GENERATION_SETTINGS_COUNT);

    GenerationSettings generationSettings;
    for (const auto &setting : settings) {
        QStringList keyValuePair = setting.split('=');
        assert(keyValuePair.size() == 2);
        const auto key = keyValuePair[0].trimmed();
        const auto value = keyValuePair[1].trimmed();

        if (key == QLatin1String("textIndexBehavoir")) {
            bool isOk = false;
            const auto tibValue = value.toInt(&isOk);
            assert(isOk && tibValue >= 0
                   && tibValue < static_cast<int>(GenerationSettings::TextIndexBehavior::None));
            generationSettings.textIndexBehavior
                = static_cast<GenerationSettings::TextIndexBehavior>(tibValue);
        }
        else if (key == QLatin1String("duplicateMouseEvent")) {
            bool isOk = false;
            const auto dmeValue = value.toInt(&isOk);
            assert(isOk && (dmeValue == 0 || dmeValue == 1));
            generationSettings.duplicateMouseEvent = dmeValue != 0;
        }
        else if (key == QLatin1String("scriptPath")) {
            generationSettings.scriptFileInfo = QFileInfo(value);
        }
    }

    assert(generationSettings.isInit());
    return generationSettings;
}
} // namespace QtAda::probe
