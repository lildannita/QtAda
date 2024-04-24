#pragma once

#include "rep_InprocessController_source.h"

namespace QtAda::inprocess {
class InprocessController : public InprocessControllerSource {
    Q_OBJECT
public:
    InprocessController(QObject *parent = nullptr) noexcept
        : InprocessControllerSource{ parent }
    {
    }

Q_SIGNALS:
    // UserVerificationFilter -> PropertiesWatcher signals:
    void newFramedRootObjectData(const QVariantMap &model, const QList<QVariantMap> &rootMetaData);
    void newMetaPropertyData(const QList<QVariantMap> &metaData);

    // PropertiesWatcher -> UserVerificationFilter signals:
    void requestFramedObjectChange(const QList<int> &rowPath);

    // UserEventFilter -> InprocessDialog signals:
    void newScriptLine(const QString &line);

    // InprocessDialog -> UserEventFilter signals:
    void scriptCompleted();
    void verificationModeChanged(bool isVerificationMode);
    void applicationPaused(bool isPaused);
    void scriptCancelled();
};
} // namespace QtAda::inprocess
