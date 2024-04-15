#pragma once

#include <optional>
#include <qnamespace.h>
#include "ProcessedObjects.hpp"

//! TODO: убрать
#include <iostream>

QT_BEGIN_NAMESPACE
class QItemSelectionModel;
QT_END_NAMESPACE

namespace QtAda::core::utils {
QString objectPath(const QObject *obj) noexcept;
template <typename GuiComponent> QString objectPath(const GuiComponent *component) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);
    auto *obj = qobject_cast<const QObject *>(component);
    assert(obj != nullptr);
    return objectPath(obj);
}

QString escapeText(const QString &text) noexcept;
QString mouseButtonToString(const Qt::MouseButton mouseButton) noexcept;
inline QString keyToString(const int key) noexcept
{
    return QKeySequence(static_cast<Qt::Key>(key)).toString();
}

template <typename GuiComponent>
bool mouseEventCanBeFiltered(const GuiComponent *component, const QMouseEvent *event,
                             bool shouldBePressEvent = false) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);
    const auto type = event->type();
    return component != nullptr && event != nullptr && event->button() == Qt::LeftButton
           && (type == (shouldBePressEvent ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease)
               || type == QEvent::MouseButtonDblClick);
}

template <typename GuiComponent>
std::pair<const GuiComponent *, size_t> searchSpecificComponentWithIteration(
    const GuiComponent *component,
    const std::pair<QLatin1String, size_t> &classDesignation) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);

    std::function<const GuiComponent *(const GuiComponent *)> parentGetter;
    if constexpr (std::is_same_v<GuiComponent, QWidget>) {
        parentGetter = [](const GuiComponent *component) { return component->parentWidget(); };
    }
    else {
        parentGetter = [](const GuiComponent *component) { return component->parentItem(); };
    }

    for (size_t i = 1; i <= classDesignation.second && component != nullptr; i++) {
        const auto *metaObject = component->metaObject();
        while (metaObject != nullptr) {
            if (classDesignation.first == metaObject->className()) {
                return std::make_pair(component, i);
            }
            metaObject = metaObject->superClass();
        }
        component = parentGetter(component);
    }
    return std::make_pair(nullptr, 0);
}
template <typename GuiComponent>
const GuiComponent *
searchSpecificComponent(const GuiComponent *component,
                        const std::pair<QLatin1String, size_t> &classDesignation) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);
    return searchSpecificComponentWithIteration(component, classDesignation).first;
}

template <typename GuiComponent, typename DigitType>
inline QString setValueStatement(const GuiComponent *component, DigitType value,
                                 std::optional<DigitType> secondValue = std::nullopt) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);
    static_assert(std::is_arithmetic<DigitType>::value, "Type T must be a digit");
    return QStringLiteral("setValue('%1', %2);")
        .arg(objectPath(component))
        .arg(secondValue.has_value() ? QStringLiteral("(%1, %2)").arg(value).arg(*secondValue)
                                     : QString::number(value));
}
template <typename GuiComponent>
inline QString setValueStatement(const GuiComponent *component, const QString &value) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);
    return QStringLiteral("setValue('%1', '%2');").arg(objectPath(component), value);
}
template <typename GuiComponent>
inline QString changeValueStatement(const GuiComponent *component, const QString &type) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);
    return QStringLiteral("changeValue('%1', '%2');").arg(objectPath(component), type);
}

template <typename T, typename GuiComponent, typename Signal, typename Slot>
QMetaObject::Connection connectIfType(const GuiComponent *sender, Signal signal,
                                      const QObject *reciever, Slot slot)
{
    CHECK_GUI_CLASS(GuiComponent);
    assert(sender != nullptr);
    assert(reciever != nullptr);
    if (auto *castedSender = qobject_cast<const T *>(sender)) {
        return QObject::connect(castedSender, signal, reciever, slot);
    }
    return {};
}

template <typename GuiComponent, typename Signal, typename Slot>
QMetaObject::Connection connectObject(const GuiComponent *sender, Signal signal,
                                      const QObject *reciever, Slot slot)
{
    assert(sender != nullptr);
    assert(reciever != nullptr);
    return QObject::connect(sender, signal, reciever, slot);
}

inline std::unique_ptr<const QEvent> cloneMouseEvent(const QEvent *event) noexcept
{
    if (auto *mouseEvent = static_cast<const QMouseEvent *>(event)) {
        return std::make_unique<const QMouseEvent>(mouseEvent->type(), mouseEvent->pos(),
                                                   mouseEvent->globalPos(), mouseEvent->button(),
                                                   mouseEvent->buttons(), mouseEvent->modifiers());
    }
    return nullptr;
}

// Special filters for QWidgets:
QString widgetIdInView(const QWidget *widget, const int index,
                       const WidgetClass widgetClass) noexcept;
QString selectedCellsData(const QItemSelectionModel *model) noexcept;
} // namespace QtAda::core::utils
