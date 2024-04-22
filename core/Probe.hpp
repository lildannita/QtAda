#pragma once

#include <QObject>
#include <vector>
#include <set>
#include <memory>

#include "GenerationSettings.hpp"

QT_BEGIN_NAMESPACE
class QTimer;
class QLocalSocket;
QT_END_NAMESPACE

namespace QtAda::core::gui {
class ControlDialog;
}

namespace QtAda::core {
class MetaObjectHandler;
class UserEventFilter;
class UserVerificationFilter;
class ScriptWriter;

class Probe : public QObject {
    Q_OBJECT

public:
    explicit Probe(const GenerationSettings &settings, QObject *parent = nullptr) noexcept;
    ~Probe() noexcept;

    static bool initialized() noexcept;
    static void initProbe(const GenerationSettings &settings) noexcept;
    static Probe *probeInstance() noexcept;

    static void startup() noexcept;
    static void addObject(QObject *obj) noexcept;
    static void removeObject(QObject *obj) noexcept;

    bool isKnownObject(QObject *obj) const noexcept;

    bool eventFilter(QObject *reciever, QEvent *event) override;

signals:
    void objectCreated(QObject *obj);
    void objectDestroyed(QObject *obj);
    void objectReparented(QObject *obj);

private slots:
    void installInternalParameters() noexcept;
    void handleObjectsQueue() noexcept;
    void kill() noexcept;

    void handleApplicationPaused(bool isPaused) noexcept;
    void handleVerificationMode(bool isMode) noexcept;

    void readLauncherMessage() noexcept;

private:
    static QAtomicPointer<Probe> s_probeInstance;
    QLocalSocket *socket_ = nullptr;

    // Очень важно, что построение дерева объектов должно происходить
    // в одном потоке из экземпляров, которые мы сохраняем в knownObjects_
    struct QueuedObject {
        QObject *obj;
        enum Type { Create, Destroy } type;

        QueuedObject(QObject *o, Type t)
            : obj(o)
            , type(t)
        {
        }
    };
    std::vector<QueuedObject> queuedObjects_;
    std::set<const QObject *> knownObjects_;
    std::vector<QObject *> reparentedObjects_;

    QTimer *queueTimer_ = nullptr;
    MetaObjectHandler *metaObjectHandler_ = nullptr;

    ScriptWriter *scriptWriter_ = nullptr;
    UserEventFilter *userEventFilter_ = nullptr;
    UserVerificationFilter *userVerificationFilter_ = nullptr;
    bool filtersPaused_ = false;
    bool verificationMode_ = false;

    std::unique_ptr<gui::ControlDialog> controlDialog_;
    const QObject *controlDialog() const noexcept;

    void addObjectAndParentsToKnown(QObject *obj) noexcept;
    void findObjectsFromCoreApp() noexcept;

    void addObjectCreationToQueue(QObject *obj) noexcept;
    void addObjectDestroyToQueue(QObject *obj) noexcept;
    void removeObjectCreationFromQueue(QObject *obj) noexcept;
    bool isObjectInCreationQueue(QObject *obj) const noexcept;
    void explicitObjectCreation(QObject *obj) noexcept;
    void notifyQueueTimer() noexcept;

    bool isIternalObject(QObject *obj) const noexcept;
    bool isStrangeClass(QObject *obj) const noexcept;

    static bool canShowWidgets() noexcept;
};
} // namespace QtAda::core
