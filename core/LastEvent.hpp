#pragma once

#include <QDateTime>
#include <QEvent>
#include <QPoint>

namespace QtAda::core {
struct LastEvent {
    QEvent::Type type = QEvent::None;
    QDateTime timestamp;
    QString objectPath;

    virtual bool registerEvent(const QString &path, const QEvent *event) noexcept;
    void clearEvent() noexcept
    {
        type = QEvent::None;
    }
};

struct LastMouseEvent : LastEvent {
    QPoint globalPos;
    Qt::MouseButtons buttons;

    bool registerEvent(const QString &path, const QEvent *event) noexcept override;
    bool isContinuous(const LastMouseEvent &pressEvent) const noexcept;
};

struct LastKeyEvent : LastEvent {
    int key = -1;

    bool registerEvent(const QString &path, const QEvent *event) noexcept override;
};
} // namespace QtAda::core
