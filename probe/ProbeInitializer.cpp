#include "ProbeInitializer.hpp"

#include <QCoreApplication>
#include <QThread>

namespace probe {
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

    // Static call of Probe::create()
    // assert (Probe::init)

    deleteLater();
}
}
