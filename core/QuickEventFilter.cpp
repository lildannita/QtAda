#include "QuickEventFilter.hpp"

#include <QQmlProperty>

//! TODO: убрать
#include <iostream>

namespace QtAda::core::filters {
// Принцип построения этого std::map смотри в WidgetEventFilter.cpp
static const std::map<QuickClass, std::pair<QLatin1String, size_t>> s_quickMetaMap = {
    { QuickClass::Button, { QLatin1String("QQuickButton"), 1 } },
    { QuickClass::TabButton, { QLatin1String("QQuickTabButton"), 1 } },
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
    { QuickClass::ComboBox, { QLatin1String("QQuickComboBox"), 1 } },
    { QuickClass::ItemDelegate, { QLatin1String("QQuickItemDelegate"), 1 } },
    { QuickClass::Tumbler, { QLatin1String("QQuickTumbler"), 3 } },
    { QuickClass::MenuBarItem, { QLatin1String("QQuickMenuBarItem"), 1 } },
    { QuickClass::MenuItem, { QLatin1String("QQuickMenuItem"), 1 } },
    { QuickClass::ItemView, { QLatin1String("QQuickItemView"), 1 } },
    { QuickClass::PathView, { QLatin1String("QQuickPathView"), 1 } },
    { QuickClass::SwipeView, { QLatin1String("QQuickSwipeView"), 2 } },
    { QuickClass::Flickable, { QLatin1String("QQuickFlickable"), 1 } },
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
        QuickClass::TabButton,
        QuickClass::RadioButton,
        QuickClass::CheckBox,
        QuickClass::Switch,
        QuickClass::MenuBarItem,
        QuickClass::MenuItem,
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

    // Для RadioButton и TabButton, хоть они и checkable, нам это не важно,
    // так как сколько по нему не кликай, он всегда будет checked.
    const auto isCheckable
        = currentClass != QuickClass::RadioButton && currentClass != QuickClass::TabButton
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

    if (!extra.isContinuous) {
        const auto upHovered = utils::getFromVariant<bool>(QQmlProperty::read(item, "up.hovered"));
        const auto downHovered
            = utils::getFromVariant<bool>(QQmlProperty::read(item, "down.hovered"));

        auto generate = [&](const QLatin1String &type) {
            return utils::changeValueStatement(
                item, QStringLiteral("%1%2")
                          .arg(event->type() == QEvent::MouseButtonDblClick ? "Dbl" : "")
                          .arg(type));
        };

        if (upHovered) {
            return generate(QLatin1String("Up"));
        }
        else if (downHovered) {
            return generate(QLatin1String("Down"));
        }
    }

    const auto value = utils::getFromVariant<int>(QQmlProperty::read(item, "value"));
    QVariant varValue;
    QMetaObject::invokeMethod(const_cast<QQuickItem *>(item), "textFromValue",
                              Q_RETURN_ARG(QVariant, varValue), Q_ARG(int, value));
    QString setValueStatement;
    if (varValue.canConvert<double>()) {
        return utils::setValueStatement(item, utils::getFromVariant<double>(varValue));
    }
    else if (varValue.canConvert<int>()) {
        return utils::setValueStatement(item, utils::getFromVariant<int>(varValue));
    }
    else if (varValue.canConvert<QString>()) {
        return utils::setValueStatement(item, utils::getFromVariant<QString>(varValue));
    }
    return utils::setValueStatement(item, value);
}

static QString qComboBoxContainerFilter(const QQuickItem *item, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::ComboBox));
    if (item == nullptr) {
        return QString();
    }

    const auto comboBoxRect = item->boundingRect();
    const auto clickPos = item->mapFromGlobal(event->globalPos());
    if (!comboBoxRect.contains(clickPos)) {
        return QString();
    }
    return QStringLiteral("// Looks like QComboBox container clicked\n// %1")
        .arg(qMouseEventHandler(item, event));
}

static QString qComboBoxFilter(const QQuickItem *item, const QMouseEvent *event,
                               const ExtraInfoForDelayed &extra) noexcept
{
    if (!utils::mouseEventCanBeFiltered(item, event, true)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::ComboBox));
    if (item == nullptr) {
        return QString();
    }

    assert(extra.changeIndex.has_value());
    const auto delegateCount = utils::getFromVariant<int>(QQmlProperty::read(item, "count"));
    assert(extra.changeIndex < delegateCount);

    QString textValue;
    QMetaObject::invokeMethod(const_cast<QQuickItem *>(item), "textAt",
                              Q_RETURN_ARG(QString, textValue), Q_ARG(int, *extra.changeIndex));
    return QStringLiteral("selectItem('%1', '%2');").arg(utils::objectPath(item)).arg(textValue);
}

static QString qTumblerFilter(const QQuickItem *item, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::Tumbler));
    if (item == nullptr) {
        return QString();
    }
    //! TODO:
    //! 1) Нужно будет учитывать, что при отпускании мыши тумблер может продолжить
    //! движение и нужно записывать то значение, на котором была произведена остановка
    //! 2) Для QML в принципе "тяжело" вытаскивать текстовое описание разных компонентов,
    //! но оно предпочительнее, чем просто индекс. В будущем надо будет реализовать поиск
    //! текстового описания элементов.
    const auto delegateCount = utils::getFromVariant<int>(QQmlProperty::read(item, "currentIndex"));
    return QStringLiteral("selectItem('%1', %2);").arg(utils::objectPath(item)).arg(delegateCount);
}

static QString qItemViewFilter(const QQuickItem *item, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::ItemView));
    if (item == nullptr) {
        return QString();
    }

    const auto clickPos = item->mapFromGlobal(event->globalPos());
    int index;
    QMetaObject::invokeMethod(const_cast<QQuickItem *>(item), "indexAt", Q_RETURN_ARG(int, index),
                              Q_ARG(double, clickPos.x()), Q_ARG(double, clickPos.y()));
    if (index == -1) {
        return qMouseEventHandler(item, event);
    }

    return QStringLiteral("delegate%1Click('%2', %3)")
        .arg(event->type() == QEvent::MouseButtonDblClick ? "Dbl" : "")
        .arg(utils::objectPath(item))
        .arg(index);
}

static QString qPathViewFilter(const QQuickItem *item, const QMouseEvent *event,
                               const ExtraInfoForDelayed &extra) noexcept
{
    Q_UNUSED(extra);
    if (!utils::mouseEventCanBeFiltered(item, event, true)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::PathView));
    if (item == nullptr) {
        return QString();
    }

    const auto index = utils::getFromVariant<int>(QQmlProperty::read(item, "currentIndex"));
    const auto count = utils::getFromVariant<int>(QQmlProperty::read(item, "count"));
    assert(index < count);
    return QStringLiteral("selectViewItem('%1', %2)").arg(utils::objectPath(item)).arg(index);
}

static QString qSwipeViewFilter(const QQuickItem *item, const QMouseEvent *event,
                                const ExtraInfoForDelayed &extra) noexcept
{
    Q_UNUSED(extra);
    if (!utils::mouseEventCanBeFiltered(item, event, true)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::SwipeView));
    if (item == nullptr) {
        return QString();
    }

    const auto index = utils::getFromVariant<int>(QQmlProperty::read(item, "currentIndex"));
    const auto count = utils::getFromVariant<int>(QQmlProperty::read(item, "count"));
    assert(index < count);
    return QStringLiteral("selectViewItem('%1', %2)").arg(utils::objectPath(item)).arg(index);
}
} // namespace QtAda::core::filters

namespace QtAda::core {
QuickEventFilter::QuickEventFilter(QObject *parent) noexcept
{
    mouseFilters_ = {
        filters::qDelayButtonFilter,
        filters::qScrollBarFilter,
        filters::qComboBoxContainerFilter,
        filters::qTumblerFilter,
        // Обязательно в таком порядке:
        filters::qButtonsFilter,
        //! TODO: Пока что вообще непонятно как правильно обрабатывать изменение индекса
        //! в QQuickSwipeView, так как он не имеет никаких сигналов об "окончании" движения
        //! или хотя бы об изменении индекса. Сигнал об окончании движения может быть в
        //! Flickable компоненте, но он не является родителем QQuickSwipeView, а может
        //! являться родителем QQuickListView, который часто и используется в качестве
        //! потомка QQuickSwipeView.
        // filters::qSwipeViewFilter,
        filters::qItemViewFilter,
    };

    signalMouseFilters_ = {
        { QuickClass::Slider, filters::qSliderFilter },
        { QuickClass::RangeSlider, filters::qRangeSliderFilter },
        { QuickClass::Dial, filters::qSliderFilter },
        { QuickClass::SpinBox, filters::qSpinBoxFilter },
        { QuickClass::ComboBox, filters::qComboBoxFilter },
        { QuickClass::PathView, filters::qPathViewFilter },
        { QuickClass::SwipeView, filters::qSwipeViewFilter },
    };

    auto &postReleaseWatchDogTimer = postReleaseWatchDog_.timer;
    postReleaseWatchDogTimer.setInterval(500);
    postReleaseWatchDogTimer.setSingleShot(true);
    connect(&postReleaseWatchDogTimer, &QTimer::timeout, this,
            &QuickEventFilter::handlePostReleaseTimeout);
}

std::pair<QString, bool> QuickEventFilter::callMouseFilters(const QObject *obj, const QEvent *event,
                                                            bool isContinuous,
                                                            bool isSpecialEvent) noexcept
{
    const std::pair<QString, bool> empty = { QString(), false };
    auto *item = qobject_cast<const QQuickItem *>(obj);
    if (item == nullptr) {
        return empty;
    }

    //! TODO: место под specificFilters

    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (mouseEvent == nullptr) {
        return empty;
    }

    const auto delayedResult = delayedWatchDog_.callDelayedFilter(item, mouseEvent, isContinuous);
    if (delayedResult.has_value() && !(*delayedResult).isEmpty()) {
        return { *delayedResult, false };
    }

    static auto generateUseless = [item, mouseEvent] {
        return std::make_pair(QStringLiteral("// Looks like useless click\n// %1")
                                  .arg(filters::qMouseEventHandler(item, mouseEvent)),
                              false);
    };

    const std::pair<QString, bool> skip = { QString(), true };
    if (postReleaseWatchDog_.isInit()) {
        if (!postReleaseWatchDog_.needToStartTimer) {
            return skip;
        }

        const auto delegateClickInfo = utils::getClickInformation(
            item, mouseEvent, filters::s_quickMetaMap.at(QuickClass::ItemDelegate));
        switch (delegateClickInfo) {
        case utils::ClickInformation::ClickInsideComponent: {
            /*
             * Этот случай для "правильного" нажатия на компонент списка: компонент
             * выбирается в качестве текущего, список закрывается -> ждем "официального"
             * сигнала от QQuickSpinBox. На всякий случай запускаем таймер, который,
             * в случае если сигнал не придет, то сгенерирует строку с обработкой
             * события нажатия мыши.
             */
            if (!postReleaseWatchDog_.isTimerActive()) {
                postReleaseWatchDog_.startTimer();
                return skip;
            }
            return empty;
        }
        case utils::ClickInformation::ClickOutsideComponent: {
            /*
             * Этот случай для зажатия компонента списка и отведения курсора в сторону:
             * компонент не выбирается, но и список не закрывается.
             */
            return generateUseless();
        }
        case utils::ClickInformation::ClickOnAnotherComponent: {
            /*
             * В качестве примера возьмем QQuickSpinBox. При зажатии на элемент выпадающего
             * списка и движении курсора (то есть по сути - скроллинг View элемента), то этот
             * элемент списка не выбирается в качестве текущего в SpinBox, но и диалог не
             * закрывается. При этом источником сигнала становится уже не ItemDelegate, а
             * сам View компонент. Соответственно, считается, что было нажатие типа
             * ClickInformation::ClickOnAnotherComponent, хотя по факту это такой же
             * бесполезный клик.
             * Так как вся механика с PostReleaseWatchDog нацелена на обработку событий
             * с QQuickItemDelegate, которые не имеют родителя, то проверяем является
             * ли текущий item потомком postReleaseWatchDog_.causedComponent (чем и должен
             * являться View компонент), и если item все-таки потомок, то считаем это бесполезным
             * действием.
             */
            if (utils::isObjectAncestor(postReleaseWatchDog_.causedComponent, item)) {
                return generateUseless();
            }
            postReleaseWatchDog_.clear();
            return empty;
        }
        default:
            Q_UNREACHABLE();
        }
    }

    for (auto &filter : mouseFilters_) {
        const auto result = filter(item, mouseEvent);
        if (!result.isEmpty()) {
            return { result, false };
        }
    }

    return empty;
}

void QuickEventFilter::setMousePressFilter(const QObject *obj, const QEvent *event) noexcept
{
    auto *item = qobject_cast<const QQuickItem *>(obj);
    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (mouseEvent == nullptr || item == nullptr
        || (item == delayedWatchDog_.causedComponent && event == delayedWatchDog_.causedEvent
            && delayedWatchDog_.causedEventType == QEvent::MouseButtonPress
            && event->type() == QEvent::MouseButtonDblClick)) {
        return;
    }

    delayedWatchDog_.clear();
    if (postReleaseWatchDog_.isInit() && !postReleaseWatchDog_.needToStartTimer) {
        postReleaseWatchDog_.clear();
    }

    PressFilterType type = PressFilterType::Default;
    auto foundQuickClass = QuickClass::None;
    std::vector<QMetaObject::Connection> connections;
    const QQuickItem *flickable = nullptr;

    const QQuickItem *currentSlider = nullptr;
    for (const auto &sliderClass : filters::s_processedSliders) {
        currentSlider
            = utils::searchSpecificComponent(item, filters::s_quickMetaMap.at(sliderClass));
        if (currentSlider != nullptr) {
            foundQuickClass = sliderClass;
            break;
        }
    }

    switch (foundQuickClass) {
    case QuickClass::Slider:
    case QuickClass::Dial:
        connections.push_back(
            QObject::connect(currentSlider, SIGNAL(moved()), this, SLOT(processSignalSlot())));
        break;
    case QuickClass::RangeSlider: {
        auto *first = QQmlProperty::read(item, "first").value<QObject *>();
        assert(first != nullptr);
        auto *second = QQmlProperty::read(item, "second").value<QObject *>();
        assert(second != nullptr);
        connections.push_back(
            QObject::connect(first, SIGNAL(moved()), this, SLOT(processSignalSlot())));
        connections.push_back(
            QObject::connect(second, SIGNAL(moved()), this, SLOT(processSignalSlot())));
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
            type = PressFilterType::Fake;
        }
        else if (auto *foundItem = utils::searchSpecificComponent(
                     item, filters::s_quickMetaMap.at(QuickClass::ComboBox))) {
            foundQuickClass = QuickClass::ComboBox;
            type = PressFilterType::PostReleaseWithDelayedTimer;
            connections.push_back(QObject::connect(foundItem, SIGNAL(activated(int)), this,
                                                   SLOT(callPostReleaseSlotWithIntArgument(int))));
        }
        else if (auto *foundItem = utils::searchSpecificComponent(
                     item, filters::s_quickMetaMap.at(QuickClass::PathView))) {
            flickable = utils::findQuickChild(foundItem,
                                              filters::s_quickMetaMap.at(QuickClass::Flickable));
            if (flickable == nullptr) {
                break;
            }
            foundQuickClass = QuickClass::PathView;
            type = PressFilterType::PostReleaseWithTimer;
            connections.push_back(QObject::connect(flickable, SIGNAL(movementStarted()), this,
                                                   SLOT(initFlickableConnection())));
        }
        else if (auto *foundItem = utils::searchSpecificComponent(
                     item, filters::s_quickMetaMap.at(QuickClass::SwipeView))) {
            flickable = utils::findQuickChild(foundItem,
                                              filters::s_quickMetaMap.at(QuickClass::Flickable));
            if (flickable == nullptr) {
                break;
            }
            foundQuickClass = QuickClass::SwipeView;
            type = PressFilterType::PostReleaseWithTimer;
            connections.push_back(QObject::connect(flickable, SIGNAL(movementStarted()), this,
                                                   SLOT(initFlickableConnection())));
            item = foundItem;
        }
        break;
    default:
        Q_UNREACHABLE();
    }

    if (foundQuickClass != QuickClass::None) {
        switch (type) {
        case PressFilterType::Default: {
            assert(utils::connectionIsInit(connections) == true);
            delayedWatchDog_.initDelay(item, mouseEvent, signalMouseFilters_.at(foundQuickClass),
                                       connections);
            break;
        }
        case PressFilterType::Fake: {
            delayedWatchDog_.initFakeDelay(item, mouseEvent,
                                           signalMouseFilters_.at(foundQuickClass));
            break;
        }
        case PressFilterType::PostReleaseWithDelayedTimer:
        case PressFilterType::PostReleaseWithTimer: {
            assert(utils::connectionIsInit(connections) == true);
            postReleaseWatchDog_.initPostRelease(
                item, mouseEvent, signalMouseFilters_.at(foundQuickClass), connections,
                type == PressFilterType::PostReleaseWithDelayedTimer, flickable);
            if (type == PressFilterType::PostReleaseWithTimer) {
                postReleaseWatchDog_.startTimer();
            }
            break;
        }
        default:
            Q_UNREACHABLE();
        }
    }
}
} // namespace QtAda::core
