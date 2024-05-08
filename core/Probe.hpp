#pragma once

#include <QObject>
#include <vector>
#include <set>
#include <memory>

#include "Settings.hpp"

QT_BEGIN_NAMESPACE
class QTimer;
class QRemoteObjectNode;
class InprocessControllerReplica;
QT_END_NAMESPACE

namespace QtAda::core {
class UserEventFilter;
class UserVerificationFilter;
class ScriptRunner;

class Probe final : public QObject {
    Q_OBJECT

public:
    explicit Probe(const LaunchType launchType, const std::optional<RecordSettings> &recordSettings,
                   const std::optional<RunSettings> &runSettings,
                   QObject *parent = nullptr) noexcept;
    ~Probe() noexcept;

    static bool initialized() noexcept;
    static void initProbe(const LaunchType launchType,
                          const std::optional<RecordSettings> &recordSettings,
                          const std::optional<RunSettings> &runSettings) noexcept;
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
    void installInternalEventFilter() noexcept;
    void handleObjectsQueue() noexcept;
    void kill() noexcept;

    void handleApplicationPaused(bool isPaused) noexcept;
    void handleVerificationMode(bool isMode) noexcept;

    void handleApplicationFinished(int exitCode) noexcept;

private:
    static QAtomicPointer<Probe> s_probeInstance;
    QTimer *queueTimer_ = nullptr;

    QRemoteObjectNode *inprocessNode_ = nullptr;
    std::shared_ptr<InprocessControllerReplica> inprocessController_ = nullptr;

    UserEventFilter *userEventFilter_ = nullptr;
    UserVerificationFilter *userVerificationFilter_ = nullptr;
    bool filtersPaused_ = false;
    bool verificationMode_ = false;

    ScriptRunner *scriptRunner_ = nullptr;
    QThread *scriptThread_ = nullptr;

    const LaunchType launchType_;

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

    const QObject *inprocessController() const noexcept;
    static bool canShowWidgets() noexcept;

    void prepareScriptRunner(const std::optional<RunSettings> &runSettings) noexcept;
};
} // namespace QtAda::core
