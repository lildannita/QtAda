#pragma once

#include <qnamespace.h>
#include <QString>
#include <QObject>

#include "ConfHandler.hpp"
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
    return QStringLiteral("%1mouse%2Click(%3, '%4', %5, %6);")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(event->type() == QEvent::MouseButtonDblClick ? "Dbl" : "")
        .arg(path.isEmpty() ? ConfHandler::getObjectId(component) : path)
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
    return QStringLiteral("%1keyEvent(%2, '%3');")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(path.isEmpty() ? ConfHandler::getObjectId(component) : path)
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
    return QStringLiteral("%1wheelEvent(%2, %3, %4);")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(path.isEmpty() ? ConfHandler::getObjectId(obj) : path)
        .arg(delta.x())
        .arg(delta.y());
}

template <typename GuiComponent>
inline QString changeValueStatement(const GuiComponent *component, const QString &type) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);
    return QStringLiteral("%1changeValue(%2, '%3');")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(ConfHandler::getObjectId(component))
        .arg(type);
}

template <typename GuiComponent, typename DigitType>
inline QString setValueStatement(const GuiComponent *component, DigitType value,
                                 std::optional<DigitType> secondValue = std::nullopt) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);
    static_assert(std::is_arithmetic<DigitType>::value, "Type T must be a digit");
    return QStringLiteral("%1setValue(%2, %3);")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(ConfHandler::getObjectId(component))
        .arg(secondValue.has_value() ? QStringLiteral("%1, %2").arg(value).arg(*secondValue)
                                     : QString::number(value));
}
template <typename GuiComponent>
inline QString setValueStatement(const GuiComponent *component, const QString &value) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);
    return QStringLiteral("%1setValue(%2, '%3');")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(ConfHandler::getObjectId(component))
        .arg(value);
}

QString buttonEventCommand(const QString &path, const QEvent *event, bool isReleaseInside,
                           const QString &buttonText = QString()) noexcept;
QString mouseAreaEventCommand(const QString &path, const QEvent *event,
                              bool isReleaseInside) noexcept;
QString checkButtonCommand(const QString &path, bool isChecked, bool isDoubleCheck,
                           const QString &buttonText = QString()) noexcept;
QString selectItemCommand(const QString &path, const QString &statement) noexcept;
QString setDelayProgressCommand(const QString &path, double progress) noexcept;
QString selectTabCommand(const QString &path, const QString &statement) noexcept;
QString treeViewCommand(const QString &path, bool isExpand, const QString &indexPath,
                        const QString &delegateText) noexcept;
QString undoCommand(const QString &path, int index, const QString &delegateText) noexcept;
QString selectViewItemCommand(const QString &path, int index) noexcept;
QString actionCommand(const QString &path, const QString &text, bool isSeparator, bool isMenu,
                      std::optional<bool> checked) noexcept;
QString delegateClickCommand(const QString &path, const QString &statement, bool isDouble,
                             const QString &text = QString()) noexcept;
QString setSelectionCommand(const QString &path, const QString &selection) noexcept;
QString clearSelectionCommand(const QString &path) noexcept;
QString setTextCommand(const QString &path, const QString &text,
                       const QString &indexPath = QString()) noexcept;
QString closeCommand(const QString &path, bool isDialog = false) noexcept;
} // namespace QtAda::core::filters
