#include "UserEventFilter.hpp"

#include <QApplication>
#include <QQuickItem>
#include <QWidget>

#include "WidgetEventFilters.hpp"
#include "utils/FilterUtils.hpp"

//! TODO: remove
#include <iostream>

namespace QtAda::core {
static constexpr qint64 EVENT_TIME_DIFF_MS = 100;

bool alreadyHandled(const QString &path, const QMouseEvent *event) noexcept;

bool isDelayed(const QString &path, const QMouseEvent *event) noexcept;

bool UserEventFilter::LastMouseEvent::registerEvent(const QString &path,
                                                    const QMouseEvent *event) noexcept
{
    const QDateTime now = QDateTime::currentDateTime();
    if (event->type() == type && std::llabs(now.msecsTo(timestamp)) < EVENT_TIME_DIFF_MS
        && event->globalPos() == globalPos && event->buttons() == buttons && path == objectPath) {
        return false;
    }

    type = event->type();
    timestamp = now;
    globalPos = event->globalPos();
    buttons = event->buttons();
    objectPath = path;

    return true;
}

bool UserEventFilter::LastMouseEvent::isDelayed(const LastMouseEvent &pressEvent) const noexcept
{
    if (type == QEvent::MouseButtonRelease && pressEvent.type == QEvent::MouseButtonPress
        && std::llabs(timestamp.msecsTo(pressEvent.timestamp)) > EVENT_TIME_DIFF_MS
        && objectPath == pressEvent.objectPath) {
        return true;
    }
    return false;
}

void UserEventFilter::LastMouseEvent::clearEvent() noexcept
{
    type = QEvent::None;
}

UserEventFilter::UserEventFilter(QObject *parent) noexcept
    : QObject{ parent }
{
    widgetFilters_ = {
        qComboBoxFilter, qSpinBoxFilter, qCheckBoxFilter,
        qButtonFilter, // Обязательно последним
    };

    //! TODO: убрать
    connect(this, &UserEventFilter::newScriptLine, this,
            [](const QString &line) { std::cout << line.toStdString() << std::endl; });
}

bool UserEventFilter::eventFilter(QObject *reciever, QEvent *event) noexcept
{
    QQuickItem *quickItem = qobject_cast<QQuickItem *>(reciever);
    QWidget *widgetItem = qobject_cast<QWidget *>(reciever);

    // Если оба указателя nullptr, то reciever - не компонент GUI
    if (quickItem == nullptr && widgetItem == nullptr) {
        return QObject::eventFilter(reciever, event);
    }

    // Компонент либо QQuickItem, либо QWidget
    assert(!(quickItem != nullptr && widgetItem != nullptr));

    //! TODO: пока что обрабатываем только QWidget
    if (quickItem != nullptr) {
        return QObject::eventFilter(reciever, event);
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        assert(mouseEvent != nullptr);
        const auto path = utils::objectPath(reciever);
        lastPressEvent_.registerEvent(path, mouseEvent);
        lastReleaseEvent_.clearEvent();
        break;
    }
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        assert(mouseEvent != nullptr);
        const auto path = utils::objectPath(reciever);

        if (!lastReleaseEvent_.registerEvent(path, mouseEvent)) {
            break;
        }

        auto scriptLine = callWidgetFilters(widgetItem, mouseEvent,
                                            lastReleaseEvent_.isDelayed(lastPressEvent_));
        if (scriptLine.isEmpty()) {
            scriptLine = qMouseEventFilter(path, widgetItem, mouseEvent);
        }
        else if (needToDuplicateMouseEvent) {
            scriptLine = QStringLiteral("%1/n%2// ")
                             .arg(scriptLine)
                             .arg(qMouseEventFilter(path, widgetItem, mouseEvent));
        }

        assert(!scriptLine.isEmpty());
        emit newScriptLine(std::move(scriptLine));
        lastPressEvent_.clearEvent();
        break;
    }
    case QEvent::KeyPress:
        break;
    case QEvent::KeyRelease:
        break;
    default:
        break;
    }

    return QObject::eventFilter(reciever, event);
}

QString UserEventFilter::callWidgetFilters(QWidget *widget, QMouseEvent *event,
                                           bool isDelayed) noexcept
{
    QString result;
    for (auto &filter : widgetFilters_) {
        result = filter(widget, event, isDelayed);
        if (!result.isEmpty()) {
            return result;
        }
    }
    return result;
}
} // namespace QtAda::core
