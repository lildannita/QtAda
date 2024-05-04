#include "Probe.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QApplication>
#include <QRemoteObjectNode>
#include <QLocalSocket>
#include <QTimer>
#include <QThread>
#include <QRecursiveMutex>
#include <QWindow>
#include <QDialog>
#include <private/qhooks_p.h>

#include "Paths.hpp"
#include "ProbeGuard.hpp"
#include "UserEventFilter.hpp"
#include "UserVerificationFilter.hpp"
#include <inprocess/rep_InprocessController_replica.h>

namespace QtAda::core {
static constexpr char QTADA_NAMESPACE[] = "QtAda::";
static constexpr uint8_t QTADA_NAMESPACE_LEN = 7;
static constexpr uint8_t LOOP_DETECTION_COUNT = 100;

//! TODO: Странный класс, который является QObject, и не совсем понятно
//! что с ним делать: у него нет ни потомков, ни родителей. Все что может
//! быть нам полезно - objectName() и metaObject()->className(). Объект
//! этого класса является "первым" источником сигнала при работе с QtWidgets.
//! Так как пока он бесполезен, то не обрабатываем его.
//!
//! UPD: кажись, это класс графической оболочки Linux
static constexpr char STRANGE_CLASS[] = "QWidgetWindow";

QAtomicPointer<Probe> Probe::s_probeInstance = QAtomicPointer<Probe>(nullptr);
//! TODO: точно ли нам нужен рекурсивный мьютекс?
Q_GLOBAL_STATIC(QRecursiveMutex, s_mutex)

struct LilProbe {
    LilProbe() = default;
    std::vector<QObject *> objsAddedBeforeProbeInit;
    bool hooksInstalled = false;

    //! TODO: нужно добавить структуру отслеживания стека вызовов,
    //! создавшего объект для понимания, чтобы отследить код, который
    //! создает структуру (StackTrace)
};
Q_GLOBAL_STATIC(LilProbe, s_lilProbe)

class AsyncCloseEvent : public QEvent {
public:
    static const QEvent::Type AsyncClose;
    AsyncCloseEvent()
        : QEvent(AsyncClose)
    {
    }
};
const QEvent::Type AsyncCloseEvent::AsyncClose
    = static_cast<QEvent::Type>(QEvent::registerEventType());

Probe::Probe(const LaunchType launchType, const std::optional<RecordSettings> &recordSettings,
             const std::optional<ExecuteSettings> &executeSettings, QObject *parent) noexcept
    : QObject{ parent }
    , queueTimer_{ new QTimer(this) }
    , initSocket_{ new QLocalSocket(this) }
    , launchType_{ launchType }
    , recordSettings_{ recordSettings }
    , executeSettings_{ executeSettings }
{
    Q_ASSERT(thread() == qApp->thread());

    queueTimer_->setSingleShot(true);
    queueTimer_->setInterval(0);
    connect(queueTimer_, &QTimer::timeout, this, &Probe::handleObjectsQueue);

    inprocessNode_ = new QRemoteObjectNode(this);
    inprocessNode_->connectToNode(QUrl(paths::REMOTE_OBJECT_PATH));
    inprocessController_.reset(inprocessNode_->acquire<InprocessControllerReplica>());
    connect(inprocessController_.get(), &QRemoteObjectReplica::notified, this, [this] {
        assert(inprocessController_->applicationRunning() == false);
        inprocessController_->pushApplicationRunning(true);
    });

    switch (launchType_) {
    case LaunchType::Record: {
        assert(recordSettings.has_value());
        userEventFilter_ = new UserEventFilter(*recordSettings, this);
        userVerificationFilter_ = new UserVerificationFilter(this);

        connect(userEventFilter_, &UserEventFilter::newScriptLine, inprocessController_.get(),
                &InprocessControllerReplica::sendNewScriptLine);
        connect(userVerificationFilter_, &UserVerificationFilter::newFramedRootObjectData,
                inprocessController_.get(),
                &InprocessControllerReplica::sendNewFramedRootObjectData);
        connect(userVerificationFilter_, &UserVerificationFilter::newMetaPropertyData,
                inprocessController_.get(), &InprocessControllerReplica::sendNewMetaPropertyData);
        connect(inprocessController_.get(), &InprocessControllerReplica::applicationPaused, this,
                &Probe::handleApplicationPaused);
        connect(inprocessController_.get(), &InprocessControllerReplica::scriptCompleted, this,
                &Probe::handleScriptCompleted);
        connect(inprocessController_.get(), &InprocessControllerReplica::scriptCancelled, this,
                &Probe::handleScriptCancelled);
        connect(inprocessController_.get(), &InprocessControllerReplica::verificationModeChanged,
                this, &Probe::handleVerificationMode);
        connect(inprocessController_.get(), &InprocessControllerReplica::requestFramedObjectChange,
                userVerificationFilter_, &UserVerificationFilter::changeFramedObject);
        break;
    }
    case LaunchType::Execute: {
        assert(executeSettings.has_value());
        //! TODO: здесь должна быть настройка для запуска тестового скрипта
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}

Probe::~Probe() noexcept
{
    qtHookData[QHooks::AddQObject] = 0;
    qtHookData[QHooks::RemoveQObject] = 0;
    qtHookData[QHooks::Startup] = 0;

    s_probeInstance = QAtomicPointer<Probe>(nullptr);
}

Probe *Probe::probeInstance() noexcept
{
    return s_probeInstance.loadRelaxed();
}

bool Probe::initialized() noexcept
{
    return probeInstance();
}

void Probe::kill() noexcept
{
    delete this;
}

void Probe::initProbe(const LaunchType launchType,
                      const std::optional<RecordSettings> &recordSettings,
                      const std::optional<ExecuteSettings> &executeSettings) noexcept
{
    assert(qApp);
    assert(!initialized());

    Probe *probe = nullptr;
    {
        ProbeGuard guard;
        probe = new Probe(launchType, recordSettings, executeSettings);
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

    QMetaObject::invokeMethod(probe, "installInternalEventFilter", Qt::QueuedConnection);
}

void Probe::startup() noexcept
{
    s_lilProbe()->hooksInstalled = true;
}

const QObject *Probe::inprocessController() const noexcept
{
    return qobject_cast<const QObject *>(inprocessController_.get());
}

bool Probe::canShowWidgets() noexcept
{
    return QCoreApplication::instance()->inherits("QApplication");
}

void Probe::installInternalEventFilter() noexcept
{
    QCoreApplication::instance()->installEventFilter(this);
}

void Probe::handleApplicationPaused(bool isPaused) noexcept
{
    if (isPaused) {
        userVerificationFilter_->cleanupFrames();
    }
    filtersPaused_ = isPaused;
}

void Probe::handleVerificationMode(bool isMode) noexcept
{
    if (!isMode) {
        userVerificationFilter_->cleanupFrames();
    }
    verificationMode_ = isMode;
}

void Probe::handleScriptCompleted() noexcept
{
    handleVerificationMode(false);
    if (recordSettings_->closeWindowsOnExit) {
        if (canShowWidgets()) {
            QApplication::closeAllWindows();
        }
        else {
            const auto windows = QGuiApplication::allWindows();
            for (auto *window : windows) {
                window->close();
            }
        }
    }
    inprocessController_->pushApplicationRunning(false);
    QCoreApplication::postEvent(QCoreApplication::instance(), new AsyncCloseEvent());
}

void Probe::handleScriptCancelled() noexcept
{
    inprocessController_->pushApplicationRunning(false);
    QCoreApplication::postEvent(QCoreApplication::instance(), new AsyncCloseEvent());
}

bool Probe::eventFilter(QObject *reciever, QEvent *event)
{
    if (ProbeGuard::locked() && reciever->thread() == QThread::currentThread()) {
        return QObject::eventFilter(reciever, event);
    }

    if (event->type() == AsyncCloseEvent::AsyncClose) {
        QCoreApplication::exit();
        return true;
    }

    if (event->type() == QEvent::ChildAdded || event->type() == QEvent::ChildRemoved) {
        QChildEvent *childEvent = static_cast<QChildEvent *>(event);
        QObject *childObj = childEvent->child();

        QMutexLocker lock(s_mutex());
        if (!isIternalObject(childObj) && childEvent->added()) {
            if (!isKnownObject(childObj)) {
                // Ситуация, когда мы узнаем о новом объекте раньше, чем при перехвате хука
                // QHooks::AddQObject, возникает из-за того, что событие QEvent::ChildAdded
                // возникает раньше хука
                addObject(childObj);
            }
            else if (!isObjectInCreationQueue(childObj)
                     && !isObjectInCreationQueue(childObj->parent())
                     && isKnownObject(childObj->parent())) {
                // Данная ситуация возникает в случае, если мы уже проинициализировали объект
                // и его нового родителя. Соответственно, нам остается только поменять позицию
                // этого объекта в дереве элементов
                reparentedObjects_.erase(
                    std::remove(reparentedObjects_.begin(), reparentedObjects_.end(), childObj),
                    reparentedObjects_.end());
                emit objectReparented(childObj);
            }
            else if (!isKnownObject(childObj->parent())) {
                // Данная ситуация возникает, когда у рассматриваемого объекта появился новый
                // родитель, о котором мы до этого ничего не знали. Соответственно, нужно добавить
                // нового родителя и обновить положение рассматриваемого объекта в дереве
                addObject(childObj->parent());
                reparentedObjects_.push_back(childObj);
                notifyQueueTimer();
            }
        }
        else if (isKnownObject(childObj)) {
            // Пока не можем понять конечное положение объекта в дереве - "откладываем его"
            reparentedObjects_.push_back(childObj);
            notifyQueueTimer();
        }
    }

    // Работает только для QWidgets
    if (event->type() == QEvent::ParentChange) {
        QMutexLocker lock(s_mutex());
        if (!isIternalObject(reciever) && isKnownObject(reciever)
            && isKnownObject(reciever->parent()) && !isObjectInCreationQueue(reciever)
            && !isObjectInCreationQueue(reciever->parent())) {
            // Данная ситуация возникает в случае, если мы уже проинициализировали объект
            // и его нового родителя. Соответственно, нам остается только поменять позицию
            // этого объекта в дереве элементов
            reparentedObjects_.erase(
                std::remove(reparentedObjects_.begin(), reparentedObjects_.end(), reciever),
                reparentedObjects_.end());
            emit objectReparented(reciever);
        }
        else if (!isKnownObject(reciever->parent())) {
            // Данная ситуация возникает, когда у рассматриваемого объекта появился новый родитель,
            // о котором мы до этого ничего не знали. Соответственно, нужно добавить нового родителя
            // и обновить положение рассматриваемого объекта в дереве
            addObject(reciever->parent());
            reparentedObjects_.push_back(reciever);
            notifyQueueTimer();
        }
    }

    /*
     * Если переопределение хуков не установлено, то и addObject/removeObject не работает,
     * из-за чего приходится на основе получаемых сигналов строить дерево элементов. Но такая
     * ситуация теоретически не может быть, однако это может быть полезно в будущем при
     * написании unit-тестов.
     */
    if (!s_lilProbe()->hooksInstalled
        // Уже обработанные события
        && event->type() != QEvent::ChildAdded && event->type() != QEvent::ChildRemoved
        && event->type() != QEvent::ParentChange
        // Сигналы, вызываемые деструкторами, которые нет смысла обрабатывать
        && event->type() != QEvent::Destroy
        && event->type() != QEvent::WinIdChange
        // Как обычно, не обрабатываем "внутренние" объекты
        && !isIternalObject(reciever)) {
        QMutexLocker lock(s_mutex());
        if (!isKnownObject(reciever)) {
            addObjectAndParentsToKnown(reciever);
        }
    }

    if (launchType_ == LaunchType::Record && !filtersPaused_ && !isIternalObject(reciever)
        && !isStrangeClass(reciever)) {
        if (verificationMode_) {
            return userVerificationFilter_->eventFilter(reciever, event);
        }
        else {
            userEventFilter_->eventFilter(reciever, event);
        }
    }

    return QObject::eventFilter(reciever, event);
}

void Probe::addObjectAndParentsToKnown(QObject *obj) noexcept
{
    if (!obj) {
        return;
    }

    QMutexLocker lock(s_mutex());
    if (isKnownObject(obj)) {
        return;
    }

    addObject(obj);
    for (auto *childObj : obj->children()) {
        addObjectAndParentsToKnown(childObj);
    }
}

void Probe::findObjectsFromCoreApp() noexcept
{
    addObjectAndParentsToKnown(QCoreApplication::instance());

    if (auto guiApp = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        for (auto *window : guiApp->allWindows()) {
            addObjectAndParentsToKnown(window);
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
    if (probeInstance()->isIternalObject(obj)) {
        return;
    }

    // Убеждаемся, что уже не обработали этот объект. Это может произойти,
    // если мы уже добавили объект из s_lilProbe->objsAddedBeforeProbeInit
    if (probeInstance()->isKnownObject(obj)) {
        return;
    }

    // Если у объекта есть непроинициализированные родители, то сразу
    // инициаилизируем их
    QObject *parent = obj->parent();
    if (parent && !probeInstance()->isKnownObject(parent)) {
        addObject(parent);
    }
    // Убеждаемся, что уже знаем о родителе объекта
    assert(!parent || probeInstance()->isKnownObject(parent));

    probeInstance()->knownObjects_.insert(obj);
    probeInstance()->addObjectCreationToQueue(obj);
}

bool Probe::isStrangeClass(QObject *obj) const noexcept
{
    return qstrcmp(obj->metaObject()->className(), STRANGE_CLASS) == 0;
}

bool Probe::isIternalObject(QObject *obj) const noexcept
{
    std::set<QObject *> checkedObjects;
    int iteration = 0;
    QObject *o = obj;
    do {
        if (iteration > LOOP_DETECTION_COUNT) {
            // Возможно закицливание в дереве, поэтому если мы уже проверяли
            // текущий объект, то считаем, что он создан QtAda
            if (checkedObjects.find(obj) != checkedObjects.end()) {
                return true;
            }
            checkedObjects.insert(o);
        }
        ++iteration;

        if (o == this || o == inprocessController()
            || (qstrncmp(o->metaObject()->className(), QTADA_NAMESPACE, QTADA_NAMESPACE_LEN)
                == 0)) {
            return true;
        }
        o = o->parent();
    } while (o);
    return false;
}

bool Probe::isKnownObject(QObject *obj) const noexcept
{
    return knownObjects_.find(obj) != knownObjects_.end();
}

void Probe::removeObject(QObject *obj) noexcept
{
    QMutexLocker lock(s_mutex());

    if (!initialized()) {
        if (!s_lilProbe()) {
            return;
        }

        auto &beforeObjects = s_lilProbe()->objsAddedBeforeProbeInit;
        auto eraseStart = std::remove(beforeObjects.begin(), beforeObjects.end(), obj);
        beforeObjects.erase(eraseStart, beforeObjects.end());
        return;
    }

    bool successErase = probeInstance()->knownObjects_.erase(obj);
    if (!successErase) {
        // Удаляемый объект не успели добавить в knownObjects, так что скорее
        // всего это объект QtAda, но дополнительно нужно проверить, что объект
        // не находится в очереди для инициализации
        assert(!probeInstance()->isObjectInCreationQueue(obj));
        return;
    }

    probeInstance()->removeObjectCreationFromQueue(obj);
    assert(!probeInstance()->isObjectInCreationQueue(obj));

    if (probeInstance()->thread() == QThread::currentThread()) {
        emit probeInstance()->objectDestroyed(obj);
    }
    else {
        probeInstance()->addObjectDestroyToQueue(obj);
    }
}

void Probe::addObjectCreationToQueue(QObject *obj) noexcept
{
    assert(!isObjectInCreationQueue(obj));

    queuedObjects_.push_back({ obj, QueuedObject::Create });
    notifyQueueTimer();
}

void Probe::addObjectDestroyToQueue(QObject *obj) noexcept
{
    queuedObjects_.push_back({ obj, QueuedObject::Destroy });
    notifyQueueTimer();
}

void Probe::removeObjectCreationFromQueue(QObject *obj) noexcept
{
    auto it = std::find_if(queuedObjects_.begin(), queuedObjects_.end(),
                           [obj](const QueuedObject &qObj) {
                               return qObj.obj == obj && qObj.type == QueuedObject::Create;
                           });
    if (it != queuedObjects_.end()) {
        queuedObjects_.erase(it);
    }
}

bool Probe::isObjectInCreationQueue(QObject *obj) const noexcept
{
    auto it = std::find_if(queuedObjects_.begin(), queuedObjects_.end(),
                           [obj](const QueuedObject &qObj) {
                               return qObj.obj == obj && qObj.type == QueuedObject::Create;
                           });
    return it != queuedObjects_.end();
}

void Probe::notifyQueueTimer() noexcept
{
    if (queueTimer_->isActive()) {
        return;
    }

    // Нам важно начать обработку отложенных объектов в одном потоке
    if (thread() == QThread::currentThread()) {
        queueTimer_->start();
    }
    else {
        // Если находимся не в текущем потоке, то откладываем запуск таймера с помощью invoke
        // (поэтому и используем таймер с нулевым интервалом, так как можем отложить
        // выполнение функции старта таймера)
        static QMetaMethod startMethod;
        if (startMethod.methodIndex() < 0) {
            const auto methodIdx = QTimer::staticMetaObject.indexOfMethod("start()");
            assert(methodIdx >= 0);
            startMethod = QTimer::staticMetaObject.method(methodIdx);
            assert(startMethod.methodIndex() >= 0);
        }
        startMethod.invoke(queueTimer_, Qt::QueuedConnection);
    }
}

void Probe::handleObjectsQueue() noexcept
{
    QMutexLocker lock(s_mutex());
    assert(thread() == QThread::currentThread());

    const auto queuedObjects = queuedObjects_;
    for (const auto &o : queuedObjects) {
        switch (o.type) {
        case QueuedObject::Create:
            explicitObjectCreation(o.obj);
            break;
        case QueuedObject::Destroy:
            emit objectDestroyed(o.obj);
            break;
        default:
            Q_UNREACHABLE();
        }
    }
    queuedObjects_.clear();

    const auto reparentedObjects = reparentedObjects_;
    for (QObject *obj : reparentedObjects_) {
        if (!isKnownObject(obj)) {
            continue;
        }
        if (isIternalObject(obj)) {
            removeObject(obj);
        }
        else {
            emit objectReparented(obj);
        }
    }
    reparentedObjects_.clear();
}

void Probe::explicitObjectCreation(QObject *obj) noexcept
{
    assert(thread() == QThread::currentThread());

    if (!isKnownObject(obj)) {
        return;
    }

    if (isIternalObject(obj)) {
        bool successErase = knownObjects_.erase(obj);
        assert(successErase == true);
        return;
    }

    //! TODO: Временное решение борьбы с "блокировкой" диалогом. Позволяет
    //! общаться с нашим ControlDialog даже когда запущен любой диалог.
    if (auto *dialog = qobject_cast<QDialog *>(obj)) {
        dialog->setWindowModality(Qt::NonModal);
    }

    for (QObject *parent = obj->parent(); parent != nullptr; parent = parent->parent()) {
        if (!isKnownObject(parent)) {
            addObject(parent);
            break;
        }
    }
    assert(!obj->parent() || isKnownObject(obj->parent()));

    emit objectCreated(obj);
}
} // namespace QtAda::core
