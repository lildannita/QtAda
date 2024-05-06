#pragma once

#include <QObject>
#include <QQuickPaintedItem>
#include <QPainter>
#include <QStandardItemModel>
#include <optional>

#include "LastEvent.hpp"

QT_BEGIN_NAMESPACE
class QWidget;
class QQuickItem;
class QFrame;
QT_END_NAMESPACE

namespace QtAda::core {
class QuickFrame final : public QQuickPaintedItem {
    Q_OBJECT
public:
    QuickFrame(QQuickItem *parent = nullptr)
        : QQuickPaintedItem{ parent }
    {
    }
    void paint(QPainter *painter) override
    {
        painter->setPen(QPen(QColor(217, 4, 41), 3));
        painter->setBrush(QColor(239, 35, 60, 127));
        painter->drawRect(this->boundingRect());
    }
};

class UserVerificationFilter final : public QObject {
    Q_OBJECT
public:
    UserVerificationFilter(QObject *parent = nullptr) noexcept
        : QObject{ parent }
    {
    }
    ~UserVerificationFilter() noexcept
    {
        cleanupFrames();
    }
    bool eventFilter(QObject *obj, QEvent *event) noexcept override;
    void cleanupFrames() noexcept;

signals:
    void newFramedRootObjectData(const QVariantMap &model, const QList<QVariantMap> &rootMetaData);
    void newMetaPropertyData(const QList<QVariantMap> &metaData);

public slots:
    void changeFramedObject(const QList<int> &rowPath) noexcept;

private:
    LastMouseEvent lastPressEvent_;
    QFrame *lastFrame_ = nullptr;
    QQuickPaintedItem *lastPaintedItem_ = nullptr;

    QStandardItemModel framedRootObjectModel_;

    void callHandlers(QObject *obj, bool isExtTrigger) noexcept;
    bool handleWidgetVerification(QWidget *widget, bool isExtTrigger) noexcept;
    bool handleItemVerification(QQuickItem *item, bool isExtTrigger) noexcept;

    std::optional<QVariantMap> updateFramedRootObjectModel(const QObject *object,
                                                           QStandardItem *parentViewItem) noexcept;
};
} // namespace QtAda::core
