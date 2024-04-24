#pragma once

#include "rep_InprocessController_source.h"

Q_DECLARE_METATYPE(std::deque<int>)
Q_DECLARE_METATYPE(QStandardItemModel)
//! TODO: надо ли дублировать с const?
Q_DECLARE_METATYPE(const std::deque<int>)
Q_DECLARE_METATYPE(const QStandardItemModel)

class InprocessController : public InprocessControllerSource {
    Q_OBJECT
public:
    InprocessController(QObject *parent = nullptr) noexcept;

Q_SIGNALS:
    // UserVerificationFilter -> PropertiesWatcher signals:
    void newFramedRootObjectData(const QStandardItemModel model, const std::vector<std::pair<QString, QString>> rootMetaData);
    void newMetaPropertyData(const QStandardItemModel model, const std::vector<std::pair<QString, QString>> metaData);

    // PropertiesWatcher -> UserVerificationFilter signals:
    void requestFramedObjectChange(const std::deque<int> rowPath);
};
