#pragma once

#include <qnamespace.h>
#include <QString>
#include <QObject>

#include "FilterUtils.hpp"
#include "Paths.hpp"

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
    return QStringLiteral("%1mouse%2Click('%3', '%4', %5, %6);")
        .arg(SCRIPT_COMMAND_PREFIX)
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
    return QStringLiteral("%1keyEvent('%2', '%3');")
        .arg(SCRIPT_COMMAND_PREFIX)
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
    return QStringLiteral("%1wheelEvent('%2', %3, %4);")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(path.isEmpty() ? utils::objectPath(obj) : path)
        .arg(delta.x())
        .arg(delta.y());
}

QString buttonEventCommand(const QString &path, const QEvent *event, bool isReleaseInside,
                           const QString &buttonText = QString()) noexcept;
QString mouseAreaEventCommand(const QString &path, const QEvent *event,
                              bool isReleaseInside) noexcept;
QString checkButtonCommand(const QString &path, bool isChecked, bool isDoubleCheck,
                           const QString &buttonText = QString()) noexcept;
QString selectItemCommand(const QString &path, const QString &statement) noexcept;
} // namespace QtAda::core::filters
