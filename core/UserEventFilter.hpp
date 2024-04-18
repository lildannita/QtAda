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
#include "QuickEventFilter.hpp"
#include "LastEvent.hpp"

QT_BEGIN_NAMESPACE
class QQuickItem;
QT_END_NAMESPACE

namespace QtAda::core {
class UserEventFilter final : public QObject {
    Q_OBJECT
public:
    UserEventFilter(const GenerationSettings &generationSettings,
                    QObject *parent = nullptr) noexcept;

    bool eventFilter(QObject *obj, QEvent *event) noexcept override;

signals:
    void newScriptLine(const QString &scriptLine) const;

private:
    LastMouseEvent lastPressEvent_;
    LastMouseEvent lastReleaseEvent_;

    std::optional<QString> delayedScriptLine_;
    std::optional<std::unique_ptr<const QMouseEvent>> delayedMouseEvent_;
    void clearDelayed()
    {
        doubleClickDetected_ = false;
        delayedScriptLine_ = std::nullopt;
        delayedMouseEvent_ = std::nullopt;
    }

    LastKeyEvent lastKeyEvent_;
    LastWheelEvent lastWheelEvent_;
    LastEvent lastFocusEvent_;

    QTimer doubleClickTimer_;
    bool doubleClickDetected_ = false;

    std::shared_ptr<WidgetEventFilter> widgetFilter_ = nullptr;
    std::shared_ptr<QuickEventFilter> quickFilter_ = nullptr;

    void flushScriptLine(const std::optional<QString> &line) const noexcept
    {
        if (line.has_value()) {
            assert(!line->isEmpty());
            emit newScriptLine(*line);
        }
    }
    MouseEventInfo mouseEventInfo(const QString &objPath = QString()) const noexcept;
};
} // namespace QtAda::core
