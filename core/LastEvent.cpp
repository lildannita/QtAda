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
    //! TODO: KeyEvent для QtQuick работает очень странно - источниками сигнала
    //! могут быть совершенно несвязанные друг с другом объекты, и вызов eventFilter()
    //! для этих объектов вызывается непоследовательно, поэтому пути не проверяем,
    //! ориентируемся только на timestamp события.
    Q_UNUSED(path);
    auto *keyEvent = static_cast<const QKeyEvent *>(event);
    if (keyEvent == nullptr) {
        return false;
    }

    const auto now = QDateTime::currentDateTime();
    if (keyEvent->type() == type && std::llabs(now.msecsTo(timestamp)) < EVENT_TIME_DIFF_MS
        && keyEvent->key() == key) {
        //! if (objectPath == path) {
        //!     return false;
        //! }
        //! const auto parentPath = objectPath.left(objectPath.lastIndexOf('/'));
        //! if (parentPath == path) {
        //!     objectPath = path;
        //!     return false;
        //! }
        return false;
    }

    type = keyEvent->type();
    timestamp = now;
    key = keyEvent->key();
    //! objectPath = path;

    return true;
}

bool LastWheelEvent::registerEvent(const QString &path, const QEvent *event) noexcept
{
    //! TODO: При WheelEvent получается, что первый источник сигнала - самый старший родитель,
    //! что очень странно, так как обычно порядок вызова eventFilter начинается с самого младшего.
    //! Поэтому если путь до источника сигнала состоит из одного компонента, то считаем, что
    //! этот источник - самый старший родитель и игнорируем событие. Однако, нужно проверить, бывает
    //! ли полезен WheelEvent для объектов типа QMainWindow.
    const auto now = QDateTime::currentDateTime();
    if ((event->type() == type && std::llabs(now.msecsTo(timestamp)) < EVENT_TIME_DIFF_MS)
        || path.lastIndexOf('/') == -1) {
        //! TODO: Для WheelEvent не проверяем objPath, из-за путей источников сигнала в QtQuick -
        //! для одного сигнала может быть куча источников, имеющих "непоследовательных" родителей
        return false;
    }

    type = event->type();
    timestamp = now;

    return true;
}
} // namespace QtAda::core
