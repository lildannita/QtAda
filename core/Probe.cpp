#include "Probe.hpp"

#include "ProbeGuard.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QThread>
#include <QRecursiveMutex>
#include <QWindow>

#include <vector>
#include <private/qhooks_p.h>

#include <iostream>

namespace QtAda::core {
static constexpr char QTADA_NAMESPACE[] = "QtAda::";
static constexpr uint8_t QTADA_NAMESPACE_LEN = 7;
static constexpr uint8_t LOOP_DETECTION_COUNT = 100;

QAtomicPointer<Probe> Probe::s_probeInstance = QAtomicPointer<Probe>(nullptr);
//! TODO: точно ли нам нужен рекурсивный мьютекс?
Q_GLOBAL_STATIC(QRecursiveMutex, s_mutex)

struct LilProbe {
    LilProbe() = default;
    std::vector<QObject *> objsAddedBeforeProbeInit;
    // bool trackDestroyed = true;

    //! TODO: нужно добавить структуру отслеживания стека вызовов,
    //! создавшего объект для понимания, чтобы отследить код, который
    //! создает структуру
};
Q_GLOBAL_STATIC(LilProbe, s_lilProbe)

Probe::Probe(QObject *parent) noexcept
    : QObject{ parent }
{
    Q_ASSERT(thread() == qApp->thread());
}

Probe::~Probe() noexcept
{
    qtHookData[QHooks::AddQObject] = 0;
    qtHookData[QHooks::RemoveQObject] = 0;
    qtHookData[QHooks::Startup] = 0;

    s_probeInstance = QAtomicPointer<Probe>(nullptr);
}

Probe *Probe::probeInstance() noexcept { return s_probeInstance.loadRelaxed(); }

bool Probe::initialized() noexcept { return probeInstance(); }

void Probe::kill() noexcept { delete this; }

void Probe::initProbe() noexcept
{
    assert(qApp);
    assert(!initialized());

    Probe *probe = nullptr;
    {
        ProbeGuard guard;
        probe = new Probe();
    }

    connect(qApp, &QCoreApplication::aboutToQuit, probe, &Probe::kill);
    connect(qApp, &QCoreApplication::destroyed, probe, &Probe::kill);

    {
        QMutexLocker lock(s_mutex());
        s_probeInstance = QAtomicPointer<Probe>(probe);

        for (QObject *obj : s_lilProbe->objsAddedBeforeProbeInit) {
            addObject(obj);
        }
        s_lilProbe->objsAddedBeforeProbeInit.clear();

        probe->findObjectsFromCoreApp();
    }

    QMetaObject::invokeMethod(probe, "installEventFilter", Qt::QueuedConnection);
}

void Probe::installEventFilter() noexcept { QCoreApplication::instance()->installEventFilter(this); }

void Probe::discoverObject(QObject *obj) noexcept
{
    if (!obj) {
        return;
    }

    QMutexLocker lock(s_mutex());
    if (knownObjects_.find(obj) != knownObjects_.end()) {
        return;
    }

    addObject(obj);
    for (auto *childObj : obj->children()) {
        discoverObject(childObj);
    }
}

void Probe::findObjectsFromCoreApp() noexcept
{
    discoverObject(QCoreApplication::instance());

    if (auto guiApp = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        for (auto *window : guiApp->allWindows()) {
            discoverObject(window);
        }
    }
}

void Probe::addObject(QObject *obj) noexcept
{
    QMutexLocker lock(s_mutex());

    // Попытка сразу игнорировать объекты, созданные QtAda
    if (ProbeGuard::locked() && obj->thread() == QThread::currentThread()) {
        return;
    }

    // Игнорируем объекты, создаваемые после закрытия QtAda
    if (s_lilProbe.isDestroyed()) {
        return;
    }

    //! TODO: StackTrace

    if (!initialized()) {
        s_lilProbe->objsAddedBeforeProbeInit.push_back(obj);
        return;
    }

    // Игнорируем объекты, имеющие сигнатуру QtAda
    if (probeInstance()->isIternalObjectCreated(obj)) {
        return;
    }

    // Убеждаемся, что уже не обработали этот объект. Это может произойти,
    // если мы уже добавили объект из s_lilProbe->objsAddedBeforeProbeInit
    if (probeInstance()->knownObjects_.find(obj) != probeInstance()->knownObjects_.end()) {
        return;
    }

    // Если у объекта есть непроинициализированные родители, то сразу
    // инициаилизируем их
    QObject *parent = obj->parent();
    if (parent && probeInstance()->knownObjects_.find(parent) == probeInstance()->knownObjects_.end()) {
        addObject(parent);
    }
    // Убеждаемся, что уже знаем о родителе объекта
    assert(!parent || probeInstance()->knownObjects_.find(parent) != probeInstance()->knownObjects_.end());

    probeInstance()->knownObjects_.insert(obj);

    //! TODO: добавляем в очередь инициализацию нового объекта
}

bool Probe::isIternalObjectCreated(QObject *obj) const noexcept
{
    std::set<QObject *> checkedObjects;
    int iteration = 0;
    QObject *o = obj;
    do {
        if (iteration > LOOP_DETECTION_COUNT) {
            // Возможно закицливание в дереве, поэтому если мы уже проверяли
            // текущий объект, то считаем, что он создан QtAda
            if (knownObjects_.find(obj) != knownObjects_.end()) {
                return true;
            }
            checkedObjects.insert(o);
        }
        ++iteration;

        //! TODO: добавить проверку на собственный интерфейс, когда он будет готов
        if (o == this || (qstrncmp(o->metaObject()->className(), QTADA_NAMESPACE, QTADA_NAMESPACE_LEN) == 0)) {
            return true;
        }
        o = o->parent();
    } while (o);
    return false;
}

void Probe::removeObject(QObject *obj) noexcept
{
    QMutexLocker lock(s_mutex());

    if (!initialized()) {
        if (!s_lilProbe())
            return;

        auto &beforeObjects = s_lilProbe()->objsAddedBeforeProbeInit;
        auto eraseStart = std::remove(beforeObjects.begin(), beforeObjects.end(), obj);
        beforeObjects.erase(eraseStart, beforeObjects.end());
        return;
    }

    bool success = probeInstance()->knownObjects_.erase(obj);
    if (!success) {
        // Удаляемый объект не успели добавить в knownObjects, так что скорее
        // всего это объект QtAda, но дополнительно нужно проверить, что объект
        // не находится в очереди для инициализации
        //! TODO: проверка объекта на нахождение в очереди
        return;
    }

    //! TODO: удаление объекта из очереди на инициализацию
}
} // namespace QtAda::core
