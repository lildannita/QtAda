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

QT_BEGIN_NAMESPACE
class QQuickItem;
QT_END_NAMESPACE

namespace QtAda::core {
class UserEventFilter final : public QObject {
    Q_OBJECT
public:
    UserEventFilter(QObject *parent = nullptr) noexcept;

    bool eventFilter(QObject *reciever, QEvent *event) noexcept override;

signals:
    void newScriptLine(const QString &scriptLine);

private:
    using WidgetEventFilter = std::function<QString(QWidget *, QMouseEvent *, bool)>;
    struct LastMouseEvent {
        QEvent::Type type = QEvent::None;
        QDateTime timestamp;
        QPoint globalPos;
        Qt::MouseButtons buttons;
        QString objectPath;

        QDateTime pushTimestamp;

        bool registerEvent(const QString &path, const QMouseEvent *event) noexcept;
        bool isDelayed(const LastMouseEvent &pressEvent) const noexcept;
        void clearEvent() noexcept;
    };

    std::vector<WidgetEventFilter> widgetFilters_;
    LastMouseEvent lastPressEvent_;
    LastMouseEvent lastReleaseEvent_;

    std::optional<QString> delayedScriptLine_;

    bool needToDuplicateMouseEvent = false;

    QTimer doubleClickTimer_;
    bool doubleClickDetected_ = false;

    QString handleMouseEvent(QString objPath, QWidget *widget, QMouseEvent *event) noexcept;
    QString callWidgetFilters(QWidget *widget, QMouseEvent *event, bool isDelayed) noexcept;
};
} // namespace QtAda::core
