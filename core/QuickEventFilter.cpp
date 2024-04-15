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
    { QuickClass::Switch, { QLatin1String("QQuickSwitch"), 1 } },
    { QuickClass::DelayButton, { QLatin1String("QQuickDelayButton"), 1 } },
    { QuickClass::Slider, { QLatin1String("QQuickSlider"), 1 } },
    { QuickClass::RangeSlider, { QLatin1String("QQuickRangeSlider"), 1 } },
    { QuickClass::Dial, { QLatin1String("QQuickDial"), 1 } },
    { QuickClass::ScrollBar, { QLatin1String("QQuickScrollBar"), 1 } },
    { QuickClass::SpinBox, { QLatin1String("QQuickSpinBox"), 1 } },
};

//! TODO: если будут использоваться только в одной функции, то перенести объявление в эти функции
static const std::vector<QuickClass> s_processedTextWidgets = {};

static const std::vector<QuickClass> s_processedSliders = {
    QuickClass::RangeSlider,
    QuickClass::Dial,
    QuickClass::Slider,
};

//! TODO: нужна ли обработка зажатия кастомной кнопки?
static QString qButtonsFilter(const QQuickItem *item, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    static const std::vector<QuickClass> processedButtons = {
        QuickClass::MouseArea,
        QuickClass::RadioButton,
        QuickClass::CheckBox,
        QuickClass::Switch,
        // Обязательно последним:
        QuickClass::Button,
    };

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
    const auto isCheckable
        = currentClass != QuickClass::RadioButton
              ? utils::getFromVariant<bool>(QQmlProperty::read(currentItem, "checkable"))
              : false;
    // Во время события Release состояние checked еще не поменяется, поэтому инвертируем значение
    const auto isChecked = !utils::getFromVariant<bool>(QQmlProperty::read(currentItem, "checked"));
    const auto buttonText = utils::getFromVariant<QString>(QQmlProperty::read(currentItem, "text"));

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
            return QStringLiteral("%1\n%2").arg(generate(!isChecked)).arg(generate(isChecked));
        }
        else {
            return generate(isChecked);
        }
    }

    return QStringLiteral("button%1('%2');%3")
        .arg(clickType())
        .arg(utils::objectPath(currentItem))
        .arg(buttonText.isEmpty() ? "" : QStringLiteral(" // Button text: '%1'").arg(buttonText));
}

static QString qDelayButtonFilter(const QQuickItem *item, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::DelayButton));
    if (item == nullptr) {
        return QString();
    }

    const auto buttonRect = item->boundingRect();
    const auto clickPos = item->mapFromGlobal(event->globalPos());
    if (buttonRect.contains(clickPos)) {
        const auto buttonProgress
            = utils::getFromVariant<double>(QQmlProperty::read(item, "progress"));
        return QStringLiteral("setDelayProgress('%1', %2);")
            .arg(utils::objectPath(item))
            .arg(buttonProgress);
    }

    return qMouseEventHandler(item, event);
}

static QString qSliderFilter(const QQuickItem *item, const QMouseEvent *event,
                             const ExtraInfoForDelayed &extra) noexcept
{
    Q_UNUSED(extra);
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    const QQuickItem *currentItem = nullptr;
    for (const auto &btnClass : s_processedSliders) {
        if (btnClass == QuickClass::RangeSlider) {
            // Обрабатывается в отдельном фильтре
            continue;
        }
        currentItem = utils::searchSpecificComponent(item, s_quickMetaMap.at(btnClass));
        if (currentItem != nullptr) {
            break;
        }
    }
    if (currentItem == nullptr) {
        return QString();
    }

    return utils::setValueStatement(
        currentItem, utils::getFromVariant<double>(QQmlProperty::read(item, "value")));
}

static QString qRangeSliderFilter(const QQuickItem *item, const QMouseEvent *event,
                                  const ExtraInfoForDelayed &extra) noexcept
{
    Q_UNUSED(extra);
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::RangeSlider));
    if (item == nullptr) {
        return QString();
    }

    const auto firstValue = utils::getFromVariant<double>(QQmlProperty::read(item, "first.value"));
    const auto secondValue
        = utils::getFromVariant<double>(QQmlProperty::read(item, "second.value"));
    return utils::setValueStatement(item, firstValue, std::make_optional(secondValue));
}

static QString qScrollBarFilter(const QQuickItem *item, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::ScrollBar));
    if (item == nullptr) {
        return QString();
    }

    return utils::setValueStatement(
        item, utils::getFromVariant<double>(QQmlProperty::read(item, "position")));
}

static QString qSpinBoxFilter(const QQuickItem *item, const QMouseEvent *event,
                              const ExtraInfoForDelayed &extra) noexcept
{
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::SpinBox));
    if (item == nullptr) {
        return QString();
    }

    //! TODO: DoubleClick неправильно обрабатывается, т.к. при двойном клике происходит следующее:
    //! (P - Press, R - Release, D - DblClick)
    //! P -> R (valueModified) -> P -> D -> R -> valueModified (сильно позже Release!)
    //!
    //! Единственное, что может "спасти" - это changeValueStatement с DblUp/DblDown, который
    //! генерируется при !extra.isContinuous. Возможно, лучше, при !extra.isContinuous все-таки
    //! не генерировать setValueStatement.
    const auto value = utils::getFromVariant<int>(QQmlProperty::read(item, "value"));
    QVariant varValue;
    QMetaObject::invokeMethod(const_cast<QQuickItem *>(item), "textFromValue",
                              Q_RETURN_ARG(QVariant, varValue), Q_ARG(int, value));
    QString setValueStatement;
    if (varValue.canConvert<double>()) {
        setValueStatement = utils::setValueStatement(item, utils::getFromVariant<double>(varValue));
    }
    else if (varValue.canConvert<int>()) {
        setValueStatement = utils::setValueStatement(item, utils::getFromVariant<int>(varValue));
    }
    else if (varValue.canConvert<QString>()) {
        setValueStatement
            = utils::setValueStatement(item, utils::getFromVariant<QString>(varValue));
    }
    else {
        setValueStatement = utils::setValueStatement(item, value);
    }

    if (!extra.isContinuous) {
        const auto upHovered = utils::getFromVariant<bool>(QQmlProperty::read(item, "up.hovered"));
        const auto downHovered
            = utils::getFromVariant<bool>(QQmlProperty::read(item, "down.hovered"));

        auto generate = [&](const QLatin1String &type) {
            return QStringLiteral("%1\n// %2")
                .arg(setValueStatement)
                .arg(utils::changeValueStatement(
                    item, QStringLiteral("%1%2")
                              .arg(event->type() == QEvent::MouseButtonDblClick ? "Dbl" : "")
                              .arg(type)));
        };

        if (upHovered) {
            return generate(QLatin1String("Up"));
        }
        else if (downHovered) {
            return generate(QLatin1String("Down"));
        }
    }

    return setValueStatement;
}
} // namespace QtAda::core::filters

namespace QtAda::core {
QuickEventFilter::QuickEventFilter(QObject *parent) noexcept
{
    mouseFilters_ = {
        filters::qDelayButtonFilter,
        filters::qScrollBarFilter,
        // Обязательно последним:
        filters::qButtonsFilter,
    };

    delayedMouseFilters_ = {
        { QuickClass::Slider, filters::qSliderFilter },
        { QuickClass::RangeSlider, filters::qRangeSliderFilter },
        { QuickClass::Dial, filters::qSliderFilter },
        { QuickClass::SpinBox, filters::qSpinBoxFilter },
    };
}

QString QuickEventFilter::callMouseFilters(const QObject *obj, const QEvent *event,
                                           bool isContinuous, bool isSpecialEvent) noexcept
{
    auto *item = qobject_cast<const QQuickItem *>(obj);
    if (item == nullptr) {
        return QString();
    }

    //! TODO: место под specificFilters

    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (mouseEvent == nullptr) {
        return QString();
    }

    const auto delayedResult = delayedData_.callDelayedFilter(item, mouseEvent, isContinuous);
    if (delayedResult.has_value() && !(*delayedResult).isEmpty()) {
        return *delayedResult;
    }

    for (auto &filter : mouseFilters_) {
        const auto result = filter(item, mouseEvent);
        if (!result.isEmpty()) {
            return result;
        }
    }
    return QString();
}

void QuickEventFilter::setMousePressFilter(const QObject *obj, const QEvent *event) noexcept
{
    auto *item = qobject_cast<const QQuickItem *>(obj);
    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (mouseEvent == nullptr || item == nullptr
        || (item == delayedData_.causedComponent && event == delayedData_.causedEvent
            && delayedData_.causedEventType == QEvent::MouseButtonPress
            && event->type() == QEvent::MouseButtonDblClick)) {
        return;
    }

    delayedData_.clear();

    auto foundQuickClass = QuickClass::None;
    std::vector<QMetaObject::Connection> connections;

    const QQuickItem *currentSlider = nullptr;
    for (const auto &sliderClass : filters::s_processedSliders) {
        currentSlider
            = utils::searchSpecificComponent(item, filters::s_quickMetaMap.at(sliderClass));
        if (currentSlider != nullptr) {
            foundQuickClass = sliderClass;
            break;
        }
    }

    bool isFakeDelay = false;
    switch (foundQuickClass) {
    case QuickClass::Slider:
    case QuickClass::Dial:
        connections.push_back(
            QObject::connect(currentSlider, SIGNAL(moved()), this, SLOT(classicCallSlot())));
        break;
    case QuickClass::RangeSlider: {
        auto *first = QQmlProperty::read(item, "first").value<QObject *>();
        assert(first != nullptr);
        auto *second = QQmlProperty::read(item, "second").value<QObject *>();
        assert(second != nullptr);
        connections.push_back(
            QObject::connect(first, SIGNAL(moved()), this, SLOT(classicCallSlot())));
        connections.push_back(
            QObject::connect(second, SIGNAL(moved()), this, SLOT(classicCallSlot())));
        break;
    }
    case QuickClass::None:
        if (auto *foundItem = utils::searchSpecificComponent(
                item, filters::s_quickMetaMap.at(QuickClass::SpinBox))) {
            /*
             * Для QQuickSpinBox бесполезно пытаться отследить сигнал об изменении, так как идет
             * сильное несовпадение по таймингу: либо практически одновременно с Press (обычный
             * клик), либо сильно позже Release (двойной клик). Но тем не менее нам важно передать в
             * эту функцию isContinuous.
             */
            foundQuickClass = QuickClass::SpinBox;
            isFakeDelay = true;
        }
        break;
    default:
        Q_UNREACHABLE();
    }

    if (foundQuickClass != QuickClass::None) {
        if (isFakeDelay) {
            delayedData_.initFakeDelay(item, mouseEvent, delayedMouseFilters_.at(foundQuickClass));
        }
        else {
            assert(delayedData_.connectionIsInit(connections) == true);
            delayedData_.initDelay(item, mouseEvent, delayedMouseFilters_.at(foundQuickClass),
                                   connections);
        }
    }
}
} // namespace QtAda::core
