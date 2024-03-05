#pragma once

#include <QObject>
#include <vector>
#include <set>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace QtAda::core {
class MetaObjectHandler;

class Probe : public QObject {
    Q_OBJECT

public:
    explicit Probe(QObject *parent = nullptr) noexcept;
    ~Probe() noexcept;

    static bool initialized() noexcept;
    static void initProbe() noexcept;
    static Probe *probeInstance() noexcept;

    static void startup() noexcept;
    static void addObject(QObject *obj) noexcept;
    static void removeObject(QObject *obj) noexcept;

    bool isKnownObject(QObject *obj) const noexcept;

    bool eventFilter(QObject *reciever, QEvent *event) override;
    void installEventFilter(QObject *filter) noexcept;

signals:
    void objectCreated(QObject *obj);
    void objectDestroyed(QObject *obj);
    void objectReparented(QObject *obj);

private slots:
    void installInternalEventFilter() noexcept;
    void handleObjectsQueue() noexcept;
    void kill() noexcept;

private:
    static QAtomicPointer<Probe> s_probeInstance;
    std::vector<QObject *> eventFilters_;

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

    void addObjectAndParentsToKnown(QObject *obj) noexcept;
    void findObjectsFromCoreApp() noexcept;

    void addObjectCreationToQueue(QObject *obj) noexcept;
    void addObjectDestroyToQueue(QObject *obj) noexcept;
    void removeObjectCreationFromQueue(QObject *obj) noexcept;
    bool isObjectInCreationQueue(QObject *obj) const noexcept;
    void explicitObjectCreation(QObject *obj) noexcept;
    void notifyQueueTimer() noexcept;

    bool isIternalObject(QObject *obj) const noexcept;
};
} // namespace QtAda::core
