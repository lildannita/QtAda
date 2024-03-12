#pragma once

#include <QObject>
#include <QHash>

namespace QtAda::core {
class MetaObjectHandler : public QObject {
    Q_OBJECT
public:
    explicit MetaObjectHandler(QObject *parent = nullptr) noexcept;
    ~MetaObjectHandler() noexcept = default;

public slots:
    void objectCreatedOutside(QObject *obj) noexcept;
    void objectDestroyedOutside(QObject *obj) noexcept;

private:
    struct MetaObjectInfo final {
        QByteArray className;
        bool isStatic = false;
        bool isDynamic = false;
        bool isInvalid = false;
        int selfCount = 0;
        int selfAliveCount = 0;
        int inclusiveCount = 0;
        int inclusiveAliveCount = 0;
    };
    QHash<const QMetaObject *, MetaObjectInfo> metaObjectsInfo_;
    QHash<QObject *, const QMetaObject *> metaObjects_;

    //! TODO: надо ли
    QHash<QObject *, const QMetaObject *> dynamicMetaObjects_;
    QHash<QByteArray, const QMetaObject *> dynamicMetaObjectNames_;

    QHash<const QMetaObject *, const QMetaObject *> childrenAndTheirParents_;
    QHash<const QMetaObject *, QVector<const QMetaObject *>> parentsAndTheirChildren_;

    QHash<const QMetaObject *, QVector<const QMetaObject *>> aliveMetaObjectInstances_;
    QHash<const QMetaObject *, const QMetaObject *> canonicalMetaObjects_;
    //!

    const QMetaObject *addMetaObject(const QMetaObject *obj,
                                     bool needToMergeDynamic = false) noexcept;
    const QMetaObject *parentOf(const QMetaObject *obj) const noexcept;

    void addAliveInstance(QObject *obj, const QMetaObject *canonicalMetaObj) noexcept;
    void removeAliveInstance(QObject *obj, const QMetaObject *canonicalMetaObj) noexcept;

    void scanMetaTypes() noexcept;
    bool isKnownMetaObject(const QMetaObject *obj) const noexcept;
};
} // namespace QtAda::core
