#pragma once

#include <rep_InprocessController_source.h>

namespace QtAda::inprocess {
class InprocessController : public InprocessControllerSimpleSource {
    Q_OBJECT
public:
    InprocessController(QObject *parent = nullptr) noexcept
        : InprocessControllerSimpleSource{ parent }
    {
    }

public Q_SLOTS:
    // UserEventFilter -> InprocessDialog slots:
    void sendNewScriptLine(const QString &line) override
    {
        emit this->newScriptLine(line);
    }

    // UserVerificationFilter -> PropertiesWatcher slots:
    void sendNewMetaPropertyData(const QList<QVariantMap> &metaData) override
    {
        emit this->newMetaPropertyData(metaData);
    }
    void sendNewFramedRootObjectData(const QVariantMap &model,
                                     const QList<QVariantMap> &rootMetaData) override
    {
        emit this->newFramedRootObjectData(model, rootMetaData);
    }

Q_SIGNALS:
    // UserEventFilter -> InprocessDialog signals:
    void newScriptLine(const QString &line);

    // UserVerificationFilter -> PropertiesWatcher signals:
    void newMetaPropertyData(const QList<QVariantMap> &metaData);
    void newFramedRootObjectData(const QVariantMap &model, const QList<QVariantMap> &rootMetaData);
};
} // namespace QtAda::inprocess
