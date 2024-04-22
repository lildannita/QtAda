#include "Probe.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QApplication>
#include <QLocalSocket>
#include <QScreen>
#include <QTimer>
#include <QThread>
#include <QRecursiveMutex>
#include <QWindow>
#include <QMetaMethod>
#include <private/qhooks_p.h>

#include "ProbeGuard.hpp"
#include "MetaObjectHandler.hpp"
#include "UserEventFilter.hpp"
#include "UserVerificationFilter.hpp"
#include "ScriptWriter.hpp"

#include "utils/SocketMessages.hpp"
#include "gui/ControlDialog.hpp"

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

Probe::Probe(const GenerationSettings &settings, QObject *parent) noexcept
    : QObject{ parent }
    , queueTimer_{ new QTimer(this) }
    , metaObjectHandler_{ new MetaObjectHandler(this) }
    , userEventFilter_{ new UserEventFilter(settings, this) }
    , userVerificationFilter_{ new UserVerificationFilter(this) }
    , scriptWriter_{ new ScriptWriter(settings, this) }
{
    Q_ASSERT(thread() == qApp->thread());

    queueTimer_->setSingleShot(true);
    queueTimer_->setInterval(0);
    connect(queueTimer_, &QTimer::timeout, this, &Probe::handleObjectsQueue);

    connect(this, &Probe::objectCreated, metaObjectHandler_,
            &MetaObjectHandler::objectCreatedOutside);
    connect(this, &Probe::objectDestroyed, metaObjectHandler_,
            &MetaObjectHandler::objectDestroyedOutside);

    connect(userEventFilter_, &UserEventFilter::newScriptLine, scriptWriter_,
            &ScriptWriter::handleNewLine);

    if (canShowWidgets()) {
        controlDialog_ = std::make_unique<gui::ControlDialog>(settings.closeWindowsOnExit());
        connect(userEventFilter_, &UserEventFilter::newScriptLine, controlDialog_.get(),
                &gui::ControlDialog::handleNewScriptLine);
        connect(controlDialog_.get(), &gui::ControlDialog::newCommentLine, scriptWriter_,
                &ScriptWriter::handleNewComment);
        connect(controlDialog_.get(), &gui::ControlDialog::scriptCancelled, scriptWriter_,
                &ScriptWriter::handleCancelledScript);
        connect(controlDialog_.get(), &gui::ControlDialog::applicationPaused, scriptWriter_,
                [this](bool isPaused) { filtersPaused_ = isPaused; });
        connect(controlDialog_.get(), &gui::ControlDialog::verificationModeChanged, scriptWriter_,
                [this](bool isVerificationMode) {
                    verificationMode_ = isVerificationMode;
                    if (!verificationMode_) {
                        userVerificationFilter_->cleanupFrames();
                    }
                });
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

void Probe::initProbe(const GenerationSettings &settings) noexcept
{
    assert(qApp);
    assert(!initialized());

    Probe *probe = nullptr;
    {
        ProbeGuard guard;
        probe = new Probe(settings);
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

    QMetaObject::invokeMethod(probe, "installInternalParameters", Qt::QueuedConnection);
}

void Probe::startup() noexcept
{
    s_lilProbe()->hooksInstalled = true;
}

const QObject *Probe::controlDialog() const noexcept
{
    return qobject_cast<const QObject *>(controlDialog_.get());
}

bool Probe::canShowWidgets() noexcept
{
    return QCoreApplication::instance()->inherits("QApplication");
}

void Probe::installInternalParameters() noexcept
{
    QCoreApplication::instance()->installEventFilter(this);

    socket_ = new QLocalSocket(this);
    socket_->connectToServer(socket::SERVER_PATH);
    connect(socket_, &QLocalSocket::disconnected, socket_, &QLocalSocket::deleteLater);
    connect(socket_, &QLocalSocket::readyRead, this, &Probe::readLauncherMessage);

    if (canShowWidgets()) {
        assert(controlDialog_ != nullptr);
        const auto screenGeometry = QApplication::primaryScreen()->geometry();
        controlDialog_->move(screenGeometry.width() - controlDialog_->width(), screenGeometry.y());
        controlDialog_->show();
    }
}

bool Probe::eventFilter(QObject *reciever, QEvent *event)
{
    if (ProbeGuard::locked() && reciever->thread() == QThread::currentThread()) {
        return QObject::eventFilter(reciever, event);
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

    if (!filtersPaused_ && !isIternalObject(reciever) && !isStrangeClass(reciever)) {
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

        if (o == this || o == controlDialog()
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
        assert(successErase);
        return;
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

void Probe::readLauncherMessage() noexcept
{
    const auto message = socket_->readAll();
    if (message == socket::LAUNCHER_DESTROYED) {
        QCoreApplication::quit();
    }
}
} // namespace QtAda::core
