#pragma once

#include <qnamespace.h>
#include <QString>
#include <QObject>

#include "FilterUtils.hpp"

namespace QtAda::core::filters {
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
        .arg(eventText.isEmpty() ? utils::keyToString(keyEvent->key())
                                 : utils::escapeText(eventText));
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
} // namespace QtAda::core::filters
