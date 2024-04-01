#pragma once

#include <QObject>
#include <QWidget>
#include <QEvent>
#include <QMouseEvent>

#include <QDateTime>
#include <QPoint>
#include <QTimer>

#include <optional>
#include <vector>
#include <queue>

#include "WidgetEventFilter.hpp"

QT_BEGIN_NAMESPACE
class QQuickItem;
QT_END_NAMESPACE

namespace QtAda::core {
class UserEventFilter final : public QObject {
    Q_OBJECT
public:
    UserEventFilter(QObject *parent = nullptr) noexcept;

    bool eventFilter(QObject *reciever, QEvent *event) noexcept override;
    void setDuplicateMouseEvent(bool value) noexcept
    {
        duplicateMouseEvent_ = value;
    }

signals:
    void newScriptLine(const QString &scriptLine) const;

private:
    struct LastMouseEvent {
        QEvent::Type type = QEvent::None;
        QDateTime timestamp;
        QPoint globalPos;
        Qt::MouseButtons buttons;
        QString objectPath;

        QDateTime pushTimestamp;

        bool registerEvent(const QString &path, const QEvent *event) noexcept;
        bool isContinuous(const LastMouseEvent &pressEvent) const noexcept;
        void clearEvent() noexcept
        {
            type = QEvent::None;
        }
    };
    LastMouseEvent lastPressEvent_;
    LastMouseEvent lastReleaseEvent_;
    QStringList delayedScriptLines_;

    QTimer doubleClickTimer_;
    bool doubleClickDetected_ = false;
    bool duplicateMouseEvent_ = false;

    WidgetEventFilter widgetFilter_;

    QStringList handleMouseEvent(const QString &objPath, const QWidget *widget, const QEvent *event,
                                 bool isSpecialEvent = false) noexcept;
    void flushScriptLines(const QStringList &list) const noexcept;
};
} // namespace QtAda::core
