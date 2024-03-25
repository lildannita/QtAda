#include "MetaObjectHandler.hpp"

#include "MetaTypeDeclarations.hpp"
#include "utils/Common.hpp"
#include "Probe.hpp"

#include <QThread>

namespace QtAda {
// Этот класс нужен для получения protected данных
class UnprotectedQObject : public QObject {
public:
    inline QObjectData *data() const
    {
        return d_ptr.data();
    }
};

static inline bool hasDynamicMetaObject(const QObject *obj)
{
    return reinterpret_cast<const UnprotectedQObject *>(obj)->data()->metaObject != nullptr;
}
} // namespace QtAda

namespace QtAda::core {
MetaObjectHandler::MetaObjectHandler(QObject *parent) noexcept
    : QObject{ parent }
{
    qRegisterMetaType<const QMetaObject *>();
    scanMetaTypes();
}

void MetaObjectHandler::scanMetaTypes() noexcept
{
    for (int typeId = 0; typeId <= QMetaType::User || QMetaType::isRegistered(typeId); ++typeId) {
        if (!QMetaType::isRegistered(typeId)) {
            continue;
        }
        const auto *metaType = QMetaType::metaObjectForType(typeId);
        if (metaType != nullptr) {
            addMetaObject(metaType);
        }
    }
    addMetaObject(&staticQtMetaObject);
}

const QMetaObject *MetaObjectHandler::parentOf(const QMetaObject *obj) const noexcept
{
    return childrenAndTheirParents_.value(obj);
}

bool MetaObjectHandler::isKnownMetaObject(const QMetaObject *obj) const noexcept
{
    return childrenAndTheirParents_.contains(obj);
}

const QMetaObject *MetaObjectHandler::addMetaObject(const QMetaObject *obj,
                                                    bool needToMergeDynamic) noexcept
{
    if (isKnownMetaObject(obj)) {
        return obj;
    }

    const QMetaObject *parentObj = obj->superClass();
    if (parentObj && !isKnownMetaObject(parentObj)) {
        parentObj = addMetaObject(parentObj, needToMergeDynamic);
    }

    const bool isObjStatic = utils::isReadOnlyData(obj);
    if (!isObjStatic && needToMergeDynamic) {
        const auto objName = QByteArray(obj->className());
        const auto savedObj = dynamicMetaObjectNames_.constFind(objName);
        if (savedObj != dynamicMetaObjectNames_.end()) {
            return *savedObj;
        }
        dynamicMetaObjectNames_.insert(objName, obj);
    }

    auto &metaObjInfo = metaObjectsInfo_[obj];
    metaObjInfo.className = obj->className();
    metaObjInfo.isStatic = isObjStatic;
    metaObjInfo.isDynamic = !isObjStatic && needToMergeDynamic;

    childrenAndTheirParents_.insert(obj, parentObj);

    auto &parentsChildren = parentsAndTheirChildren_[parentObj];
    //! TODO: emit before adding
    parentsChildren.push_back(obj);
    //! TODO: emit after adding
    return obj;
}

void MetaObjectHandler::objectCreatedOutside(QObject *obj) noexcept
{
    assert(QThread::currentThread() == thread());
    assert(Probe::probeInstance()->isKnownObject(obj));
    assert(!obj->parent() || Probe::probeInstance()->isKnownObject(obj->parent()));

    const auto *metaObj = obj->metaObject();
    metaObj = addMetaObject(metaObj, hasDynamicMetaObject(obj));
    metaObjects_.insert(obj, metaObj);

    auto &metaObjInfo = metaObjectsInfo_[metaObj];
    metaObjInfo.selfCount++;
    metaObjInfo.selfAliveCount++;
    if (metaObjInfo.isDynamic) {
        addAliveInstance(obj, metaObj);
    }

    const auto *currentMetaObj = metaObj;
    while (currentMetaObj != nullptr) {
        auto &currentMetaObjInfo = metaObjectsInfo_[currentMetaObj];
        currentMetaObjInfo.inclusiveCount++;
        currentMetaObjInfo.inclusiveAliveCount++;
        currentMetaObjInfo.isInvalid = false;
        //! TODO: emit dataChanged();
        currentMetaObj = parentOf(currentMetaObj);
    }
}

void MetaObjectHandler::objectDestroyedOutside(QObject *obj) noexcept
{
    assert(QThread::currentThread() == thread());

    const auto *metaObj = metaObjects_.take(obj);
    if (metaObj == nullptr) {
        return;
    }

    auto &metaObjInfo = metaObjectsInfo_[metaObj];
    assert(!metaObjInfo.className.isEmpty());

    metaObjInfo.selfAliveCount--;
    assert(metaObjInfo.selfAliveCount >= 0);
    if (metaObjInfo.isDynamic) {
        removeAliveInstance(obj, metaObj);
    }

    const auto *currentMetaObj = metaObj;
    while (currentMetaObj != nullptr) {
        auto &currentMetaObjInfo = metaObjectsInfo_[currentMetaObj];
        currentMetaObjInfo.inclusiveAliveCount--;
        assert(currentMetaObjInfo.inclusiveAliveCount >= 0);
        //! TODO: emit dataChanged();
        if (currentMetaObjInfo.inclusiveAliveCount == 0 && !currentMetaObjInfo.isStatic) {
            currentMetaObjInfo.isInvalid = true;
        }
        currentMetaObj = parentOf(currentMetaObj);
    }
}

void MetaObjectHandler::addAliveInstance(QObject *obj, const QMetaObject *canonicalMetaObj) noexcept
{
    auto aliveMetaObj = obj->metaObject();
    dynamicMetaObjects_.insert(obj, aliveMetaObj);
    canonicalMetaObjects_.insert(aliveMetaObj, canonicalMetaObj);
    auto &aliveInstances = aliveMetaObjectInstances_[canonicalMetaObj];
    auto it = std::lower_bound(aliveInstances.begin(), aliveInstances.end(), aliveMetaObj);
    aliveInstances.insert(it, aliveMetaObj);
}

void MetaObjectHandler::removeAliveInstance(QObject *obj,
                                            const QMetaObject *canonicalMetaObj) noexcept
{
    auto aliveMetaObj = dynamicMetaObjects_.take(obj);
    auto &aliveInstances = aliveMetaObjectInstances_[canonicalMetaObj];
    auto savedInstance = std::find(aliveInstances.begin(), aliveInstances.end(), aliveMetaObj);
    if (savedInstance != aliveInstances.end()) {
        aliveInstances.erase(savedInstance);
    }
    canonicalMetaObjects_.remove(aliveMetaObj);
}
} // namespace QtAda::core
