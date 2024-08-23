#pragma once

#include <optional>
#include <qnamespace.h>

#include "Settings.hpp"
#include "ProcessedObjects.hpp"

//! TODO: remove
#include "ConfHandler.hpp"

QT_BEGIN_NAMESPACE
class QItemSelectionModel;
QT_END_NAMESPACE

namespace QtAda::core {
enum class ChangeType {
    Up = 0,
    DblUp,
    Down,
    DblDown,
    SingleStepAdd,
    SingleStepSub,
    PageStepAdd,
    PageStepSub,
    ToMinimum,
    ToMaximum,
};
}

namespace QtAda::core::utils {
//! TODO: remove
inline QString objectPath(const QObject *obj) noexcept
{
    return ConfHandler::getObjectId(obj);
}
template <typename GuiComponent> QString objectPath(const GuiComponent *component) noexcept
{
    return ConfHandler::getObjectId(component);
}

QString escapeText(const QString &text) noexcept;
QString mouseButtonToString(const Qt::MouseButton mouseButton) noexcept;
inline QString keyToString(const int key) noexcept
{
    return QKeySequence(static_cast<Qt::Key>(key)).toString();
}
QString changeTypeToString(const ChangeType type) noexcept;

std::optional<Qt::MouseButton> mouseButtonFromString(const QString &mouseButton) noexcept;
std::optional<ChangeType> changeTypeFromString(const QString &changeType) noexcept;

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

inline bool metaObjectWatcher(const QMetaObject *metaObject,
                              const QLatin1String &className) noexcept
{
    while (metaObject != nullptr) {
        if (className == metaObject->className()) {
            return true;
        }
        metaObject = metaObject->superClass();
    }
    return false;
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
        //! TODO: Почему-то parentItem() часто возвращает каких-то "странных" родителей: либо не
        //! тех, либо не всех. Причем текущий вариант делает по факту то же самое, но работает
        //! правильнее. Нужно потестировать и понять в чем разница.
        //! parentGetter = [](const GuiComponent *component) { return component->parentItem(); };
        parentGetter = [](const GuiComponent *component) {
            return qobject_cast<const QQuickItem *>(component->parent());
        };
    }

    for (size_t i = 1; i <= classDesignation.second && component != nullptr; i++) {
        if (metaObjectWatcher(component->metaObject(), classDesignation.first)) {
            return std::make_pair(component, i);
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

template <typename T, typename Signal, typename Slot>
QMetaObject::Connection connectObject(const T *sender, Signal signal, const QObject *reciever,
                                      Slot slot)
{
    assert(sender != nullptr);
    assert(reciever != nullptr);
    return QObject::connect(sender, signal, reciever, slot);
}

inline std::unique_ptr<const QMouseEvent> cloneMouseEvent(const QEvent *event) noexcept
{
    if (auto *mouseEvent = static_cast<const QMouseEvent *>(event)) {
        return std::make_unique<const QMouseEvent>(mouseEvent->type(), mouseEvent->pos(),
                                                   mouseEvent->globalPos(), mouseEvent->button(),
                                                   mouseEvent->buttons(), mouseEvent->modifiers());
    }
    return nullptr;
}

template <typename Type> Type getFromVariant(const QVariant &variant) noexcept
{
    static_assert(std::is_same_v<Type, int> || std::is_same_v<Type, double>
                      || std::is_same_v<Type, bool> || std::is_same_v<Type, QString>,
                  "Unsupported type");

    assert(variant.canConvert<Type>());
    if constexpr (std::is_same_v<Type, int>) {
        return variant.toInt();
    }
    else if constexpr (std::is_same_v<Type, double>) {
        return variant.toDouble();
    }
    else if constexpr (std::is_same_v<Type, bool>) {
        return variant.toBool();
    }
    else if constexpr (std::is_same_v<Type, QString>) {
        return variant.toString();
    }
    Q_UNREACHABLE();
}

enum class ClickInformation {
    ClickInsideComponent,
    ClickOutsideComponent,
    ClickOnAnotherComponent,
    None,
};

template <typename GuiComponent>
ClickInformation
getClickInformation(const GuiComponent *component, const QMouseEvent *event,
                    const std::pair<QLatin1String, size_t> &classDesignation) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);
    if (!mouseEventCanBeFiltered(component, event)) {
        return ClickInformation::None;
    }

    component = searchSpecificComponent(component, classDesignation);
    if (component == nullptr) {
        return ClickInformation::ClickOnAnotherComponent;
    }

    const auto delegateRect = component->boundingRect();
    const auto clickPos = component->mapFromGlobal(event->globalPos());
    if (delegateRect.contains(clickPos)) {
        return ClickInformation::ClickInsideComponent;
    }
    return ClickInformation::ClickOutsideComponent;
}

inline bool connectionIsInit(const std::vector<QMetaObject::Connection> &connections) noexcept
{
    for (auto &connection : connections) {
        if (connection) {
            return true;
        }
    }
    return false;
}

template <typename GuiComponent>
bool isObjectAncestor(const GuiComponent *ancestorComponent,
                      const GuiComponent *descendantComponent)
{
    CHECK_GUI_CLASS(GuiComponent);
    auto *ancestor = qobject_cast<const QObject *>(ancestorComponent);
    assert(ancestor != nullptr);
    auto *descendant = qobject_cast<const QObject *>(descendantComponent);
    assert(descendant != nullptr);

    while (descendant != nullptr) {
        if (descendant == ancestor) {
            return true;
        }
        descendant = descendant->parent();
    }
    return false;
}

QString textIndexStatement(TextIndexBehavior behavior, int index,
                           const QString &text = QString()) noexcept;

// Special filters for QWidgets:
QString selectedCellsData(const QItemSelectionModel *model) noexcept;
QString treeIndexPath(const QModelIndex &index) noexcept;
} // namespace QtAda::core::utils
