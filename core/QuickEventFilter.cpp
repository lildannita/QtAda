#include "QuickEventFilter.hpp"

#include <QQmlProperty>
#include <QJSValue>
#include <QQmlEngine>

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
    { QuickClass::TextInput, { QLatin1String("QQuickTextInput"), 1 } },
    { QuickClass::TextEdit, { QLatin1String("QQuickTextEdit"), 1 } },
    //! { QuickClass::TextField, { QLatin1String("QQuickTextField"), 1 } },
    //! { QuickClass::TextArea, { QLatin1String("QQuickTextArea"), 1 } },
    { QuickClass::Window, { QLatin1String("QQuickWindow"), 1 } },
    // Нужно для QuickEventFilter::processKeyEvent
    { QuickClass::ExtSpinBox, { QLatin1String("QQuickSpinBox"), 2 } },
    { QuickClass::ExtComboBox, { QLatin1String("QQuickComboBox"), 2 } },
};

static const std::vector<QuickClass> s_processedTextItems = {
    //! QuickClass::TextArea,
    //! QuickClass::TextField,
    QuickClass::TextEdit,
    QuickClass::TextInput,
};

static const std::vector<QuickClass> s_processedSliders = {
    QuickClass::RangeSlider,
    QuickClass::Dial,
    QuickClass::Slider,
};

//! TODO: нужна ли обработка зажатия кастомной кнопки?
static QString qButtonsFilter(const QQuickItem *item, const QMouseEvent *event,
                              const RecordSettings &settings) noexcept
{
    Q_UNUSED(settings);
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

    const auto buttonPath = utils::objectPath(currentItem);
    const auto buttonRect = currentItem->boundingRect();
    const auto clickPos = currentItem->mapFromGlobal(event->globalPos());
    const auto rectContains = buttonRect.contains(clickPos);

    if (currentClass == QuickClass::MouseArea) {
        return mouseAreaEventCommand(buttonPath, event, rectContains);
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
        return checkButtonCommand(buttonPath, isChecked,
                                  event->type() == QEvent::MouseButtonDblClick, buttonText);
    }
    else {
        return buttonEventCommand(buttonPath, event, rectContains, buttonText);
    }
}

static QString qDelayButtonFilter(const QQuickItem *item, const QMouseEvent *event,
                                  const RecordSettings &settings) noexcept
{
    Q_UNUSED(settings);
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
        return setDelayProgressCommand(
            utils::objectPath(item),
            utils::getFromVariant<double>(QQmlProperty::read(item, "progress")));
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

    return setValueStatement(currentItem,
                             utils::getFromVariant<double>(QQmlProperty::read(item, "value")));
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
    return setValueStatement(item, firstValue, std::make_optional(secondValue));
}

static QString qScrollBarFilter(const QQuickItem *item, const QMouseEvent *event,
                                const RecordSettings &settings) noexcept
{
    Q_UNUSED(settings);
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::ScrollBar));
    if (item == nullptr) {
        return QString();
    }

    return setValueStatement(item,
                             utils::getFromVariant<double>(QQmlProperty::read(item, "position")));
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
        if (upHovered) {
            return changeValueStatement(
                item, utils::changeTypeToString(event->type() == QEvent::MouseButtonDblClick
                                                    ? ChangeType::DblUp
                                                    : ChangeType::Up));
        }
        else if (downHovered) {
            return changeValueStatement(
                item, utils::changeTypeToString(event->type() == QEvent::MouseButtonDblClick
                                                    ? ChangeType::DblDown
                                                    : ChangeType::Down));
        }
    }

    const auto value = utils::getFromVariant<int>(QQmlProperty::read(item, "value"));
    auto rawValueFromText = QQmlProperty::read(item, "textFromValue");
    if (rawValueFromText.canConvert<QJSValue>()) {
        auto valueFromText = rawValueFromText.value<QJSValue>();
        if (valueFromText.isCallable()) {
            auto *originalEngine = qmlEngine(item);
            assert(originalEngine != nullptr);
            auto locale = QLocale::system();
            auto jsLocale = originalEngine->toScriptValue(locale);

            auto varValue = valueFromText.call(QJSValueList() << value << jsLocale).toVariant();
            if (varValue.canConvert<double>()) {
                return setValueStatement(item, utils::getFromVariant<double>(varValue));
            }
            else if (varValue.canConvert<int>()) {
                return setValueStatement(item, utils::getFromVariant<int>(varValue));
            }
            else if (varValue.canConvert<QString>()) {
                return setValueStatement(item, utils::getFromVariant<QString>(varValue));
            }
        }
    }
    return setValueStatement(item, value);
}

static QString qComboBoxContainerFilter(const QQuickItem *item, const QMouseEvent *event,
                                        const RecordSettings &settings) noexcept
{
    Q_UNUSED(settings);
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
    return selectItemCommand(utils::objectPath(item),
                             utils::textIndexStatement(extra.recordSettings.textIndexBehavior,
                                                       *extra.changeIndex, textValue));
}

static QString qTumblerFilter(const QQuickItem *item, const QMouseEvent *event,
                              const RecordSettings &settings) noexcept
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
    const auto currentIndex = utils::getFromVariant<int>(QQmlProperty::read(item, "currentIndex"));
    const auto textIndexBehavior = settings.textIndexBehavior;
    return QStringLiteral("%1%2")
        .arg(textIndexBehavior == TextIndexBehavior::OnlyText
                     || textIndexBehavior == TextIndexBehavior::TextIndex
                 ? "// This QtAda version can't take text from this GUI component\n"
                 : "")
        .arg(selectItemCommand(utils::objectPath(item),
                               utils::textIndexStatement(textIndexBehavior, currentIndex)));
}

static QString qItemViewFilter(const QQuickItem *item, const QMouseEvent *event,
                               const RecordSettings &settings) noexcept
{
    Q_UNUSED(settings);
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

    return delegateClickCommand(utils::objectPath(item), QString::number(index),
                                event->type() == QEvent::MouseButtonDblClick);
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
    return selectViewItemCommand(utils::objectPath(item), index);
}

static QString qSwipeViewFilter(const QQuickItem *item, const QMouseEvent *event,
                                const RecordSettings &settings) noexcept
{
    Q_UNUSED(settings);
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::SwipeView));
    if (item == nullptr) {
        return QString();
    }

    //! TODO: Пока эта обработка работает "плохо". Для правильной генерации пользователю
    //! нужно четко довести мышью до полного отображения нужной страницы. Идея правильной
    //! обработки этого компонента, а также и для PathView, лежит в ветке feature-flickablefilter.

    const auto index = utils::getFromVariant<int>(QQmlProperty::read(item, "currentIndex"));
    const auto count = utils::getFromVariant<int>(QQmlProperty::read(item, "count"));
    assert(index < count);
    return QStringLiteral("// It's better to checkout setted index.\n%1")
        .arg(selectViewItemCommand(utils::objectPath(item), index));
}

static QString qTextFocusFilters(const QQuickItem *item, const QMouseEvent *event,
                                 const RecordSettings &settings) noexcept
{
    Q_UNUSED(settings);
    for (const auto &quickClass : s_processedTextItems) {
        if (auto *foundWidget
            = utils::searchSpecificComponent(item, s_quickMetaMap.at(quickClass))) {
            QLatin1String quickClassStr;
            switch (quickClass) {
            case QuickClass::TextInput:
                quickClassStr = QLatin1String("TextInput");
                break;
            case QuickClass::TextEdit:
                quickClassStr = QLatin1String("TextEdit");
                break;
            default:
                Q_UNREACHABLE();
            }

            return QStringLiteral("// Looks like focus click on %1\n// %2")
                .arg(quickClassStr)
                .arg(filters::qMouseEventHandler(foundWidget, event));
        }
    }
    return QString();
}
} // namespace QtAda::core::filters

namespace QtAda::core {
QuickEventFilter::QuickEventFilter(const RecordSettings &settings, QObject *parent) noexcept
    : GuiEventFilter{ settings, parent }
    , postReleaseWatchDog_{ settings }
{
    mouseFilters_ = {
        filters::qDelayButtonFilter,
        filters::qScrollBarFilter,
        filters::qComboBoxContainerFilter,
        filters::qTumblerFilter,
        // Обязательно в таком порядке:
        filters::qButtonsFilter,
        filters::qSwipeViewFilter,
        filters::qItemViewFilter,
        filters::qTextFocusFilters,
    };

    signalMouseFilters_ = {
        { QuickClass::Slider, filters::qSliderFilter },
        { QuickClass::RangeSlider, filters::qRangeSliderFilter },
        { QuickClass::Dial, filters::qSliderFilter },
        { QuickClass::SpinBox, filters::qSpinBoxFilter },
        { QuickClass::ComboBox, filters::qComboBoxFilter },
        { QuickClass::PathView, filters::qPathViewFilter },
    };

    auto &postReleaseWatchDogTimer = postReleaseWatchDog_.timer;
    postReleaseWatchDogTimer.setInterval(500);
    postReleaseWatchDogTimer.setSingleShot(true);
    connect(&postReleaseWatchDogTimer, &QTimer::timeout, this,
            &QuickEventFilter::handlePostReleaseTimeout);
}

std::pair<QString, bool> QuickEventFilter::callMouseFilters(const QObject *obj, const QEvent *event,
                                                            bool isContinuous) noexcept
{
    const std::pair<QString, bool> empty = { QString(), false };
    auto *item = qobject_cast<const QQuickItem *>(obj);
    if (item == nullptr) {
        return empty;
    }

    // Считаем, что любое нажатие мышью обозначает конец редактирования текста
    callKeyFilters();

    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (mouseEvent == nullptr) {
        return empty;
    }

    const auto delayedResult = delayedWatchDog_.callDelayedFilter(item, mouseEvent, isContinuous);
    if (delayedResult.has_value() && !(*delayedResult).isEmpty()) {
        return { *delayedResult, false };
    }

    for (auto &filter : mouseFilters_) {
        const auto result = filter(item, mouseEvent, recordSettings_);
        if (!result.isEmpty()) {
            return { result, false };
        }
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
            type = PressFilterType::PostReleaseWithTimer;
            connections.push_back(QObject::connect(foundItem, SIGNAL(activated(int)), this,
                                                   SLOT(callPostReleaseSlotWithIntArgument(int))));
        }
        else if (auto *foundItem = utils::searchSpecificComponent(
                     item, filters::s_quickMetaMap.at(QuickClass::PathView))) {
            foundQuickClass = QuickClass::PathView;
            type = PressFilterType::PostReleaseWithoutTimer;
            connections.push_back(QObject::connect(foundItem, SIGNAL(movementEnded()), this,
                                                   SLOT(callPostReleaseSlotWithEmptyArgument())));
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
        case PressFilterType::PostReleaseWithTimer:
        case PressFilterType::PostReleaseWithoutTimer: {
            assert(utils::connectionIsInit(connections) == true);
            postReleaseWatchDog_.initPostRelease(
                item, mouseEvent, signalMouseFilters_.at(foundQuickClass), connections,
                type == PressFilterType::PostReleaseWithTimer);
            break;
        }
        default:
            Q_UNREACHABLE();
        }
    }
}

void QuickEventFilter::handleKeyEvent(const QObject *obj, const QEvent *event) noexcept
{
    auto *item = qobject_cast<const QQuickItem *>(obj);
    if (item == nullptr || event == nullptr) {
        return;
    }

    // Изменение фокуса (при условии, что фокус переводится с уже зарегестрированного объекта)
    // считаем сигналом о завершении редактирования текста
    if (event->type() == QEvent::FocusAboutToChange && item == keyWatchDog_.component) {
        callKeyFilters();
    }

    if (event->type() != QEvent::KeyPress) {
        return;
    }

    if (keyWatchDog_.component != item) {
        callKeyFilters();
    }

    for (const auto &quickClass : filters::s_processedTextItems) {
        if (auto *foundItem
            = utils::searchSpecificComponent(item, filters::s_quickMetaMap.at(quickClass))) {
            keyWatchDog_.component = foundItem;
            keyWatchDog_.componentClass = quickClass;
            keyWatchDog_.timer.start();
            return;
        }
    }

    flushKeyEvent(filters::qKeyEventHandler(item, event));
}

void QuickEventFilter::callKeyFilters() noexcept
{
    if (keyWatchDog_.component == nullptr || keyWatchDog_.componentClass == QuickClass::None) {
        return;
    }

    switch (keyWatchDog_.componentClass) {
    case QuickClass::TextInput:
    case QuickClass::TextEdit: {
        const auto text
            = utils::getFromVariant<QString>(QQmlProperty::read(keyWatchDog_.component, "text"));
        processKeyEvent(std::move(text));
        return;
    }
    default:
        Q_UNREACHABLE();
    }
}

void QuickEventFilter::processKeyEvent(const QString &text) noexcept
{
    auto *item = keyWatchDog_.component;
    assert(item != nullptr);

    // Этот код нужен потому, что установка текста прямо в поле QQuickSpinBox или QQuickComboBox
    // ломает эти компоненты: для QQuickSpinBox перестают работать кнопки `+` и `-`, а для
    // QQuickComboBox - перестает работать выбор элемента в списке. Поэтому нужно получать
    // путь не до текстового поля внутри этих компонентов, а путь до самих компонентов
    const auto &comboBoxExtInfo = filters::s_quickMetaMap.at(QuickClass::ExtComboBox);
    const auto &spinBoxExtInfo = filters::s_quickMetaMap.at(QuickClass::ExtSpinBox);
    auto comboBoxSearch = utils::searchSpecificComponentWithIteration(item, comboBoxExtInfo);
    auto spinBoxSearch = utils::searchSpecificComponentWithIteration(item, spinBoxExtInfo);
    bool needToUseParent = (comboBoxSearch.first != nullptr || spinBoxSearch.first != nullptr)
                           && (comboBoxSearch.second == comboBoxExtInfo.second
                               || spinBoxSearch.second == spinBoxExtInfo.second);

    //! TODO: Как и для QtWidgets, для QtQuick желательно учитывать, если текстовый
    //! элемент находится в View компоненте, но так как для делегатов "трудно" получать
    //! родителя, то пока откладываем.
    flushKeyEvent(
        filters::setTextCommand(utils::objectPath(needToUseParent ? item->parent() : item),
                                utils::escapeText(std::move(text))));
    keyWatchDog_.clear();
}

std::optional<QString> QuickEventFilter::handleCloseEvent(const QObject *obj,
                                                          const QEvent *event) noexcept
{
    if (obj == nullptr || event == nullptr
        || (event != nullptr && event->type() != QEvent::Close)) {
        return QString();
    }

    if (utils::metaObjectWatcher(obj->metaObject(),
                                 filters::s_quickMetaMap.at(QuickClass::Window).first)) {
        return filters::closeCommand(utils::objectPath(obj));
    }
    //! TODO: Временная заглушка, см. WidgetEventFilter::handleCloseEvent()
    return std::nullopt;
}
} // namespace QtAda::core
