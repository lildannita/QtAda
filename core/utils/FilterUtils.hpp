#pragma once

#include <qnamespace.h>
#include "ProcessedObjects.hpp"

QT_BEGIN_NAMESPACE
// class QWidget;
// class QMouseEvent;
class QItemSelectionModel;
// class QLatin1String;
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
inline QString setValueStatement(const GuiComponent *component, DigitType value) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);
    static_assert(std::is_arithmetic<DigitType>::value, "Type T must be a digit");
    return QStringLiteral("setValue('%1', %2);").arg(objectPath(component)).arg(value);
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
QMetaObject::Connection connectIfType(const GuiComponent *component, const QObject *parent,
                                      Signal signal, Slot slot)
{
    CHECK_GUI_CLASS(GuiComponent);
    if (auto *castedWidget = qobject_cast<const T *>(component)) {
        return QObject::connect(castedWidget, signal, parent, slot);
    }
    return {};
}

// Special filters for QWidgets:
QString widgetIdInView(const QWidget *widget, const int index,
                       const WidgetClass widgetClass) noexcept;
QString selectedCellsData(const QItemSelectionModel *model) noexcept;
} // namespace QtAda::core::utils
