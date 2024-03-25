#include "UserEventFilter.hpp"

#include <QApplication>
#include <QQuickItem>
#include <QWidget>

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

bool UserEventFilter::LastMouseEvent::isDelayed(const LastMouseEvent &pressEvent) const noexcept
{
    if (type == QEvent::MouseButtonRelease
        && (pressEvent.type == QEvent::MouseButtonPress
            || pressEvent.type == QEvent::MouseButtonDblClick)
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
    doubleClickTimer_.setSingleShot(true);
    connect(&doubleClickTimer_, &QTimer::timeout, this, [this]() {
        if (delayedScriptLine_.has_value()) {
            assert(!delayedScriptLine_->isEmpty());
            emit newScriptLine(*delayedScriptLine_);
            delayedScriptLine_.reset();
        }
    });

    widgetFilters_ = {
        qComboBoxFilter, qCheckBoxFilter,
        qButtonFilter, // Обязательно последним
    };

    //! TODO: убрать
    connect(this, &UserEventFilter::newScriptLine, this,
            [](const QString &line) { std::cout << line.toStdString() << std::endl; });
}

QString UserEventFilter::handleMouseEvent(QString objPath, QWidget *widget,
                                          QMouseEvent *event) noexcept
{
    auto scriptLine
        = callWidgetFilters(widget, event, lastReleaseEvent_.isDelayed(lastPressEvent_));
    if (scriptLine.isEmpty()) {
        scriptLine = qMouseEventFilter(objPath, widget, event);
    }
    else if (needToDuplicateMouseEvent) {
        scriptLine = QStringLiteral("%1\n// %2")
                         .arg(scriptLine)
                         .arg(qMouseEventFilter(objPath, widget, event));
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
            if (!lastPressEvent_.registerEvent(path, mouseEvent)) {
                break;
            }
            lastReleaseEvent_.clearEvent();

            if (doubleClickTimer_.isActive() && delayedScriptLine_.has_value()) {
                assert(!delayedScriptLine_->isEmpty());
                emit newScriptLine(*delayedScriptLine_);
                delayedScriptLine_.reset();
            }
            doubleClickTimer_.start(QApplication::doubleClickInterval());
            delayedHandler_.findAndSetDelayedFilter(widgetItem, mouseEvent);
            break;
        }
        case QEvent::MouseButtonRelease: {
            if (!lastReleaseEvent_.registerEvent(path, mouseEvent)) {
                break;
            }

            if (doubleClickTimer_.isActive()) {
                delayedScriptLine_ = handleMouseEvent(path, widgetItem, mouseEvent);
            }
            else {
                if (doubleClickDetected_) {
                    doubleClickDetected_ = false;
                    if (lastReleaseEvent_.isDelayed(lastPressEvent_)) {
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
            delayedHandler_.findAndSetDelayedFilter(widgetItem, mouseEvent);
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

QString UserEventFilter::callWidgetFilters(QWidget *widget, QMouseEvent *event,
                                           bool isDelayed) noexcept
{
    const auto delayedResult = delayedHandler_.callDelayedFilter(widget, event, isDelayed);
    if (delayedResult.has_value() && !(*delayedResult).isEmpty()) {
        return *delayedResult;
    }

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
