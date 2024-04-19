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

    Probe::initProbe(GenerationSettings());
    assert(Probe::initialized());

    deleteLater();
}
} // namespace QtAda::probe
