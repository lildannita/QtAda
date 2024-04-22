#pragma once

#include <QObject>
#include <QQuickPaintedItem>
#include <QPainter>

#include "LastEvent.hpp"

QT_BEGIN_NAMESPACE
class QWidget;
class QQuickItem;
class QFrame;
QT_END_NAMESPACE

namespace QtAda::core {
class QuickFrame : public QQuickPaintedItem {
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
    void objectSelected(const QObject *obj);

private:
    LastMouseEvent lastPressEvent_;
    QFrame *lastFrame_ = nullptr;
    QQuickPaintedItem *lastPaintedItem_ = nullptr;

    void handleWidgetVerification(QWidget *widget) noexcept;
    void handleItemVerification(QQuickItem *item) noexcept;
};
} // namespace QtAda::core
