#include "LastEvent.hpp"

#include <QMouseEvent>

namespace QtAda::core {
static constexpr qint64 EVENT_TIME_DIFF_MS = 100;

bool LastEvent::registerEvent(const QString &path, const QEvent *event) noexcept
{
    const auto now = QDateTime::currentDateTime();
    if (event->type() == type && std::llabs(now.msecsTo(timestamp)) < EVENT_TIME_DIFF_MS) {
        if (objectPath == path) {
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
    objectPath = path;

    return true;
}

bool LastMouseEvent::registerEvent(const QString &path, const QEvent *event) noexcept
{
    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (mouseEvent == nullptr) {
        return false;
    }

    const auto now = QDateTime::currentDateTime();
    if (mouseEvent->type() == type && std::llabs(now.msecsTo(timestamp)) < EVENT_TIME_DIFF_MS
        && mouseEvent->buttons() == buttons) {
        if (mouseEvent->globalPos() == globalPos && objectPath == path) {
            return false;
        }
        const auto parentPath = objectPath.left(objectPath.lastIndexOf('/'));
        if (parentPath == path) {
            objectPath = path;
            return false;
        }
    }

    type = mouseEvent->type();
    timestamp = now;
    globalPos = mouseEvent->globalPos();
    buttons = mouseEvent->buttons();
    objectPath = path;

    return true;
}

bool LastMouseEvent::isContinuous(const LastMouseEvent &pressEvent) const noexcept
{
    return type == QEvent::MouseButtonRelease
           && (pressEvent.type == QEvent::MouseButtonPress
               || pressEvent.type == QEvent::MouseButtonDblClick)
           && std::llabs(timestamp.msecsTo(pressEvent.timestamp)) > EVENT_TIME_DIFF_MS
           && objectPath == pressEvent.objectPath;
}

bool LastKeyEvent::registerEvent(const QString &path, const QEvent *event) noexcept
{
    auto *keyEvent = static_cast<const QKeyEvent *>(event);
    if (keyEvent == nullptr) {
        return false;
    }

    const auto now = QDateTime::currentDateTime();
    if (keyEvent->type() == type && std::llabs(now.msecsTo(timestamp)) < EVENT_TIME_DIFF_MS
        && keyEvent->key() == key) {
        if (objectPath == path) {
            return false;
        }
        const auto parentPath = objectPath.left(objectPath.lastIndexOf('/'));
        if (parentPath == path) {
            objectPath = path;
            return false;
        }
    }

    type = keyEvent->type();
    timestamp = now;
    key = keyEvent->key();
    objectPath = path;

    return true;
}
} // namespace QtAda::core
