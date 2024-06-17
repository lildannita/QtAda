#include "ProbeInitializer.hpp"

#include <QCoreApplication>
#include <QThread>
#include <QFileInfo>

#include "Probe.hpp"
#include "Settings.hpp"
#include <config.h>

namespace QtAda::probe {
using namespace core;
ProbeInitializer::ProbeInitializer() noexcept
{
    moveToThread(QCoreApplication::instance()->thread());
    if (qgetenv(ENV_UNSET_PRELOAD) == "1") {
        qputenv("LD_PRELOAD", "");
    }

    QMetaObject::invokeMethod(this, "initProbe", Qt::QueuedConnection);
}

void ProbeInitializer::initProbe() noexcept
{
    if (!qApp) {
        deleteLater();
        return;
    }

    assert(QThread::currentThread() == qApp->thread());

#ifdef DEBUG_RECORD
    RecordSettings settings;
    settings.scriptPath = "/tmp/record_debug.js";
    Probe::initProbe(LaunchType::Record, settings, std::nullopt);
#endif

#ifdef DEBUG_RUN
    RunSettings settings;
    settings.scriptPath
        = "/files/work/avia_planetable/qt_gui/tests/gui_scenarios/test_close_application.js";
    settings.showElapsed = true;
    Probe::initProbe(LaunchType::Run, std::nullopt, settings);
#endif

#ifndef DEBUG_BUILD
    const auto rawLaunchType = qgetenv(ENV_LAUNCH_TYPE);
    qputenv(ENV_LAUNCH_TYPE, "");
    assert(!rawLaunchType.isEmpty());

    bool isOk = false;
    const auto intLaunchType = rawLaunchType.toInt(&isOk);
    assert(isOk = true);
    const auto launchType = static_cast<LaunchType>(intLaunchType);

    const auto rawLaunchSettings = qgetenv(ENV_LAUNCH_SETTINGS);
    qputenv(ENV_LAUNCH_SETTINGS, "");
    assert(!rawLaunchSettings.isEmpty());

    switch (launchType) {
    case LaunchType::Record: {
        Probe::initProbe(launchType, RecordSettings::fromJson(rawLaunchSettings), std::nullopt);
        break;
    }
    case LaunchType::Run: {
        Probe::initProbe(launchType, std::nullopt, RunSettings::fromJson(rawLaunchSettings));
        break;
    }
    default:
        Q_UNREACHABLE();
    }
#endif

    assert(Probe::initialized());
    deleteLater();
}
} // namespace QtAda::probe
