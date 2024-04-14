#include "QuickEventFilter.hpp"

#include <QQmlProperty>

//! TODO: убрать
#include <iostream>

namespace QtAda::core::filters {
// Принцип построения этого std::map смотри в WidgetEventFilter.cpp
static const std::map<QuickClass, std::pair<QLatin1String, size_t>> s_quickMetaMap = {
    { QuickClass::Button, { QLatin1String("QQuickButton"), 1 } },
    { QuickClass::MouseArea, { QLatin1String("QQuickMouseArea"), 1 } },
    { QuickClass::RadioButton, { QLatin1String("QQuickRadioButton"), 1 } },
    { QuickClass::CheckBox, { QLatin1String("QQuickCheckBox"), 1 } },
};

//! TODO: если будут использоваться только в одной функции, то перенести объявление в эти функции
static const std::vector<QuickClass> s_processedTextWidgets = {};
static const std::vector<QuickClass> processedButtons = {
    QuickClass::Button,
    QuickClass::MouseArea,
    QuickClass::RadioButton,
    QuickClass::CheckBox,
};

//! TODO: нужна ли обработка зажатия кастомной кнопки?
static QString qButtonsFilter(const QQuickItem *item, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    QuickClass currentClass = QuickClass::None;
    const QQuickItem *currentItem = nullptr;
    for (const auto &btnClass : processedButtons) {
        currentItem = utils::searchSpecificComponent(item, s_quickMetaMap.at(btnClass));
        if (currentItem != nullptr) {
            currentClass = btnClass;
            break;
        }
    }
    if (currentClass == QuickClass::None) {
        assert(currentItem == nullptr);
        return QString();
    }
    assert(currentItem != nullptr);

    const auto buttonRect = currentItem->boundingRect();
    const auto clickPos = currentItem->mapFromGlobal(event->globalPos());
    const auto rectContains = buttonRect.contains(clickPos);
    auto clickType = [rectContains, event] {
        return rectContains ? (event->type() == QEvent::MouseButtonDblClick ? "DblClick" : "Click")
                            : "Press";
    };

    if (currentClass == QuickClass::MouseArea) {
        return QStringLiteral("mouseArea%1('%2');")
            .arg(clickType())
            .arg(utils::objectPath(currentItem));
    }

    // Для QRadioButton, хоть он и checkable, нам это не важно, так как сколько по нему не кликай,
    // он всегда будет checked.
    const auto isCheckable = currentClass != QuickClass::RadioButton
                                 ? QQmlProperty::read(currentItem, "checkable").toBool()
                                 : false;
    const auto isChecked = QQmlProperty::read(currentItem, "checked").toBool();
    const auto buttonText = QQmlProperty::read(currentItem, "text").toString();

    if (rectContains && isCheckable) {
        const auto buttonPath = utils::objectPath(currentItem);
        auto generate = [buttonPath, buttonText](bool isChecked) {
            return QStringLiteral("checkButton('%1', %2);%3")
                .arg(buttonPath)
                .arg(isChecked ? "true" : "false")
                .arg(buttonText.isEmpty()
                         ? ""
                         : QStringLiteral(" // Button text: '%1'").arg(buttonText));
        };

        if (event->type() == QEvent::MouseButtonDblClick) {
            return QStringLiteral("%1\n%2").arg(generate(isChecked)).arg(generate(!isChecked));
        }
        else {
            return generate(!isChecked);
        }
    }

    return QStringLiteral("button%1('%2');%3")
        .arg(clickType())
        .arg(utils::objectPath(currentItem))
        .arg(buttonText.isEmpty() ? "" : QStringLiteral(" // Button text: '%1'").arg(buttonText));
}
} // namespace QtAda::core::filters

namespace QtAda::core {
QuickEventFilter::QuickEventFilter(QObject *parent) noexcept
{
    mouseFilters_ = {
        filters::qButtonsFilter,
    };
}

QString QuickEventFilter::callMouseFilters(const QObject *obj, const QEvent *event,
                                           bool isContinuous, bool isSpecialEvent) noexcept
{
    auto *item = qobject_cast<const QQuickItem *>(obj);
    if (item == nullptr) {
        return QString();
    }

    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (mouseEvent == nullptr) {
        return QString();
    }

    for (auto &filter : mouseFilters_) {
        const auto result = filter(item, mouseEvent);
        if (!result.isEmpty()) {
            return result;
        }
    }
    return QString();
}
} // namespace QtAda::core
