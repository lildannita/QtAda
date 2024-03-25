#include "UserEventFilter.hpp"

#include <QApplication>
#include <QQuickItem>
#include <QWidget>

#include "utils/FilterUtils.hpp"

//! TODO: remove
#include <iostream>

namespace QtAda::core {
static constexpr qint64 EVENT_TIME_DIFF_MS = 100;

bool UserEventFilter::LastMouseEvent::registerEvent(const QString &path,
                                                    const QMouseEvent *event) noexcept
{
    const auto now = QDateTime::currentDateTime();
    if (event->type() == type && std::llabs(now.msecsTo(timestamp)) < EVENT_TIME_DIFF_MS
        && event->buttons() == buttons) {
        if (event->globalPos() == globalPos && objectPath == path) {
            return false;
        }
        const auto parentPath = objectPath.left(objectPath.lastIndexOf('/'));
        if (parentPath == path) {
            objectPath = path;
            return false;
        }
    }

    type = event->type();
    timestamp = now;
    globalPos = event->globalPos();
    buttons = event->buttons();
    objectPath = path;

    return true;
}

bool UserEventFilter::LastMouseEvent::isContinuous(const LastMouseEvent &pressEvent) const noexcept
{
    return type == QEvent::MouseButtonRelease
           && (pressEvent.type == QEvent::MouseButtonPress
               || pressEvent.type == QEvent::MouseButtonDblClick)
           && std::llabs(timestamp.msecsTo(pressEvent.timestamp)) > EVENT_TIME_DIFF_MS
           && objectPath == pressEvent.objectPath;
}

UserEventFilter::UserEventFilter(QObject *parent) noexcept
    : QObject{ parent }
{
    doubleClickTimer_.setSingleShot(true);
    connect(&doubleClickTimer_, &QTimer::timeout, this, [this]() {
        if (delayedScriptLine_.has_value()) {
            assert(!delayedScriptLine_->isEmpty());
            emit newScriptLine(*delayedScriptLine_);
            delayedScriptLine_.reset();
        }
    });

    //! TODO: убрать
    connect(this, &UserEventFilter::newScriptLine, this,
            [](const QString &line) { std::cout << line.toStdString() << std::endl; });
}

QString UserEventFilter::handleMouseEvent(const QString &objPath, const QWidget *widget,
                                          const QMouseEvent *event) const noexcept
{
    auto scriptLine = widgetFilter_.callWidgetFilters(
        widget, event, lastReleaseEvent_.isContinuous(lastPressEvent_));
    if (scriptLine.isEmpty()) {
        scriptLine = filters::qMouseEventFilter(objPath, widget, event);
    }
    else if (duplicateMouseEvent_) {
        scriptLine = QStringLiteral("%1\n// %2")
                         .arg(scriptLine)
                         .arg(filters::qMouseEventFilter(objPath, widget, event));
    }
    assert(!scriptLine.isEmpty());
    return scriptLine;
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
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick: {
        const auto path = utils::objectPath(reciever);
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        assert(mouseEvent != nullptr);

        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            if (!lastPressEvent_.registerEvent(std::move(path), mouseEvent)) {
                break;
            }
            lastReleaseEvent_.clearEvent();

            if (doubleClickTimer_.isActive() && delayedScriptLine_.has_value()) {
                emit newScriptLine(*delayedScriptLine_);
                delayedScriptLine_.reset();
            }
            doubleClickTimer_.start(QApplication::doubleClickInterval());
            widgetFilter_.findAndSetDelayedFilter(widgetItem, mouseEvent);
            break;
        }
        case QEvent::MouseButtonRelease: {
            if (!lastReleaseEvent_.registerEvent(path, mouseEvent)) {
                break;
            }

            if (doubleClickTimer_.isActive()) {
                delayedScriptLine_ = handleMouseEvent(std::move(path), widgetItem, mouseEvent);
            }
            else {
                if (doubleClickDetected_) {
                    doubleClickDetected_ = false;
                    if (lastReleaseEvent_.isContinuous(lastPressEvent_)) {
                        emit newScriptLine(
                            handleMouseEvent(std::move(path), widgetItem, mouseEvent));
                    }
                    else if (delayedScriptLine_.has_value()) {
                        emit newScriptLine(*delayedScriptLine_);
                    }
                }
                else {
                    emit newScriptLine(handleMouseEvent(std::move(path), widgetItem, mouseEvent));
                }
                delayedScriptLine_.reset();
            }
            lastPressEvent_.clearEvent();
            break;
        }
        case QEvent::MouseButtonDblClick: {
            if (!lastPressEvent_.registerEvent(path, mouseEvent)) {
                break;
            }
            widgetFilter_.findAndSetDelayedFilter(widgetItem, mouseEvent);
            doubleClickTimer_.stop();
            doubleClickDetected_ = true;
            delayedScriptLine_ = handleMouseEvent(std::move(path), widgetItem, mouseEvent);
            lastReleaseEvent_.clearEvent();
            break;
        }
        default:
            Q_UNREACHABLE();
        }
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
} // namespace QtAda::core
