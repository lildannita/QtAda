#pragma once

#include <qnamespace.h>
#include <QString>
#include <QObject>

#include <ProcessedObjects.hpp>

QT_BEGIN_NAMESPACE
class QMouseEvent;
QT_END_NAMESPACE

namespace QtAda::core::utils {
QString escapeText(const QString &text) noexcept;
QString objectPath(const QObject *obj) noexcept;
QString mouseButtonToString(const Qt::MouseButton mouseButton) noexcept;
inline QString keyToString(const int key) noexcept
{
    return QKeySequence(static_cast<Qt::Key>(key)).toString();
}

bool mouseEventCanBeFiltered(const QMouseEvent *event, bool shouldBePressEvent = false) noexcept;

template <typename T> inline QString setValueStatement(const QObject *obj, T value) noexcept
{
    static_assert(std::is_arithmetic<T>::value, "Type T must be a digit");
    return QStringLiteral("setValue('%1', %2);").arg(objectPath(obj)).arg(value);
}

inline QString setValueStatement(const QObject *obj, const QString &value) noexcept
{
    return QStringLiteral("setValue('%1', '%2');").arg(objectPath(obj), value);
}

inline QString changeValueStatement(const QObject *obj, const QString &type) noexcept
{
    return QStringLiteral("changeValue('%1', '%2');").arg(objectPath(obj), type);
}

template <typename T, typename Signal, typename Slot>
QMetaObject::Connection connectIfType(const QObject *widget, const QObject *parent, Signal signal,
                                      Slot slot)
{
    if (auto *castedWidget = qobject_cast<const T *>(widget)) {
        return QObject::connect(castedWidget, signal, parent, slot);
    }
    return {};
}

template <typename GuiComponent>
QString qMouseEventHandler(const GuiComponent *component, const QEvent *event,
                           const QString &path = QString()) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);

    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (component == nullptr || mouseEvent == nullptr) {
        return QString();
    }

    const auto clickPosition = component->mapFromGlobal(mouseEvent->globalPos());
    return QStringLiteral("mouse%1Click('%2', '%3', %4, %5);")
        .arg(event->type() == QEvent::MouseButtonDblClick ? "Dbl" : "")
        .arg(path.isEmpty() ? utils::objectPath(component) : path)
        .arg(utils::mouseButtonToString(mouseEvent->button()))
        .arg(clickPosition.x())
        .arg(clickPosition.y());
}

inline QString qMouseEventHandler(const QObject *obj, const QEvent *event,
                                  const QString &path = QString()) noexcept
{
    if (auto *widget = qobject_cast<const QWidget *>(obj)) {
        return qMouseEventHandler(widget, event, path);
    }
    else if (auto *item = qobject_cast<const QQuickItem *>(obj)) {
        return qMouseEventHandler(item, event, path);
    }
    else {
        Q_UNREACHABLE();
    }
}

template <typename GuiComponent>
QString qKeyEventHandler(const GuiComponent *component, const QEvent *event,
                         const QString &path = QString()) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);

    auto *keyEvent = static_cast<const QKeyEvent *>(event);
    if (component == nullptr || keyEvent == nullptr) {
        return QString();
    }

    const auto eventText = keyEvent->text();
    return QStringLiteral("keyEvent('%1', '%2');")
        .arg(path.isEmpty() ? utils::objectPath(component) : path)
        .arg(eventText.isEmpty() ? keyToString(keyEvent->key()) : utils::escapeText(eventText));
}

inline QString qWheelEventHandler(const QObject *obj, const QEvent *event,
                                  const QString &path = QString()) noexcept
{
    auto *wheelEvent = static_cast<const QWheelEvent *>(event);
    if (obj == nullptr || wheelEvent == nullptr) {
        return QString();
    }

    const auto delta = wheelEvent->pixelDelta();
    return QStringLiteral("wheelEvent('%1', (%2, %3));")
        .arg(path.isEmpty() ? utils::objectPath(obj) : path)
        .arg(delta.x())
        .arg(delta.y());
}
} // namespace QtAda::core::utils
