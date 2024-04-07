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
#include "LastEvent.hpp"

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
    LastMouseEvent lastPressEvent_;
    LastMouseEvent lastReleaseEvent_;
    std::optional<QString> delayedScriptLine_;

    LastKeyEvent lastKeyEvent_;
    LastEvent lastFocusEvent_;

    QTimer doubleClickTimer_;
    bool doubleClickDetected_ = false;
    bool duplicateMouseEvent_ = false;

    WidgetEventFilter widgetFilter_;

    QString handleMouseEvent(const QString &objPath, const QWidget *widget, const QEvent *event,
                             bool isSpecialEvent = false) noexcept;
    void flushScriptLine(const QString &line) const noexcept;
};
} // namespace QtAda::core
