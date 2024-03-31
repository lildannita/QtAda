#include "WidgetEventFilter.hpp"

#include <QString>
#include <QObject>
#include <QMouseEvent>
#include <map>

#include <QAbstractButton>
#include <QRadioButton>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QDial>
#include <QListView>
#include <QTableView>
#include <QCalendarWidget>
#include <QMenu>
#include <QAction>
#include <QTabBar>

#include "utils/Common.hpp"
#include "utils/FilterUtils.hpp"

//! TODO: remove
#include <iostream>

namespace QtAda::core::filters {
/*
 * Число - это максимальная степень вложенности нужного класса относительно изначально
 * обрабатываемого. Если обрабатываемый класс, например, - кнопка, то она и вызывает
 * eventFilter, поэтому и при обработке исследуемого указателя должны считать, что он
 * и является кнопкой, следовательно и ставим число 1. А если компонент более сложный,
 * то есть имеет несколько потомков, то eventFilter может вызвать один из его потомков.
 * Следовательно, для его правильной обработки нам нужно до него "добраться" проверив
 * N потомков.
 */
static const std::map<WidgetClass, std::pair<QLatin1String, size_t>> s_widgetMetaMap = {
    { Button, { QLatin1String("QAbstractButton"), 1 } },
    { RadioButton, { QLatin1String("QRadioButton"), 1 } },
    { CheckBox, { QLatin1String("QCheckBox"), 1 } },
    { Slider, { QLatin1String("QAbstractSlider"), 1 } },
    { ComboBox, { QLatin1String("QComboBox"), 4 } },
    { SpinBox, { QLatin1String("QAbstractSpinBox"), 1 } },
    { Calendar, { QLatin1String("QCalendarView"), 2 } },
    { Menu, { QLatin1String("QMenu"), 1 } },
    { TabBar, { QLatin1String("QTabBar"), 1 } },
};

QString qMouseEventFilter(const QString &path, const QWidget *widget,
                          const QMouseEvent *event) noexcept
{
    if (path.isEmpty() || widget == nullptr || event == nullptr) {
        return QString();
    }

    const auto clickPosition = widget->mapFromGlobal(event->globalPos());
    return QStringLiteral("%1('%2', '%3', %4, %5)")
        .arg(event->type() == QEvent::MouseButtonDblClick ? "mouseDblClick" : "mouseClick")
        .arg(path)
        .arg(utils::mouseButtonToString(event->button()))
        .arg(clickPosition.x())
        .arg(clickPosition.y());
}

//! TODO: нужна ли обработка зажатия кастомной кнопки?
static QString qButtonFilter(const QWidget *widget, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    //! TODO: скорее всего нужно будет уточнять какие именно классы, а не просто QAbstractButton
    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::Button));
    if (widget == nullptr) {
        return QString();
    }

    auto *button = qobject_cast<const QAbstractButton *>(widget);
    assert(button != nullptr);
    const auto buttonRect = button->rect();
    const auto clickPos = button->mapFromGlobal(event->globalPos());

    return QStringLiteral("%1Button('%2')%3")
        .arg(buttonRect.contains(clickPos) ? "click" : "press")
        .arg(utils::objectPath(widget))
        .arg(button->text().isEmpty()
                 ? ""
                 : QStringLiteral(" // Button text: '%1'").arg(button->text()));
}

static QString qRadioButtonFilter(const QWidget *widget, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::RadioButton));
    if (widget == nullptr) {
        return QString();
    }

    //! TODO: не лучший вариант проверки нажатия, нужно придумать лучше
    const auto fitSize = widget->minimumSizeHint();
    const auto checkBoxSize = widget->size();
    if (fitSize.isValid() && checkBoxSize.isValid()) {
        const auto clickableArea
            = QRect(QPoint(0, 0), QSize(std::min(fitSize.width(), checkBoxSize.width()),
                                        std::min(fitSize.height(), checkBoxSize.height())));
        const auto clickPos = widget->mapFromGlobal(event->globalPos());
        if (clickableArea.contains(clickPos)) {
            auto *radioButton = qobject_cast<const QRadioButton *>(widget);
            assert(radioButton != nullptr);
            return QStringLiteral("clickButton('%1')%2")
                .arg(utils::objectPath(widget))
                .arg(radioButton->text().isEmpty()
                         ? ""
                         : QStringLiteral(" // Button text: '%1'").arg(radioButton->text()));
        }
    }
    return qMouseEventFilter(utils::objectPath(widget), widget, event);
}

static QString qCheckBoxFilter(const QWidget *widget, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::CheckBox));
    if (widget == nullptr) {
        return QString();
    }

    //! TODO: не лучший вариант проверки нажатия, нужно придумать лучше
    const auto fitSize = widget->minimumSizeHint();
    const auto checkBoxSize = widget->size();
    if (fitSize.isValid() && checkBoxSize.isValid()) {
        const auto clickableArea
            = QRect(QPoint(0, 0), QSize(std::min(fitSize.width(), checkBoxSize.width()),
                                        std::min(fitSize.height(), checkBoxSize.height())));
        const auto clickPos = widget->mapFromGlobal(event->globalPos());
        if (clickableArea.contains(clickPos)) {
            auto *checkBox = qobject_cast<const QCheckBox *>(widget);
            assert(checkBox != nullptr);
            //! TODO: разобраться с tristate
            return QStringLiteral("checkButton('%1', %2)%3")
                .arg(utils::objectPath(widget))
                .arg(checkBox->isChecked() ? "false" : "true")
                .arg(checkBox->text().isEmpty()
                         ? ""
                         : QStringLiteral(" // Button text: '%1'").arg(checkBox->text()));
            ;
        }
    }
    return qMouseEventFilter(utils::objectPath(widget), widget, event);
}

static QString qComboBoxFilter(const QWidget *widget, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    size_t iteration;
    std::tie(widget, iteration) = utils::searchSpecificWidgetWithIteration(
        widget, s_widgetMetaMap.at(WidgetClass::ComboBox));
    if (widget == nullptr) {
        return QString();
    }
    if (iteration <= 2) {
        return QStringLiteral("// Looks like QComboBox container clicked\n// %1")
            .arg(qMouseEventFilter(utils::objectPath(widget), widget, event));
    }

    auto *comboBox = qobject_cast<const QComboBox *>(widget);
    assert(comboBox != nullptr);

    //! TODO: нужно проверить, если выполнение кода дошло до этого места, то точно ли
    //! был нажат элемент списка
    auto *comboBoxView = comboBox->view();
    const auto containerRect = comboBoxView->rect();
    const auto clickPos = comboBoxView->mapFromGlobal(event->globalPos());

    if (containerRect.contains(clickPos)) {
        return QStringLiteral("selectItem('%1', '%2')")
            .arg(utils::objectPath(comboBox))
            .arg(utils::widgetIdInView(comboBox, comboBoxView->currentIndex().row(),
                                       WidgetClass::ComboBox));
    }
    /*
     * Отпускание мыши не приведет к закрытию QListView, и если мы зарегестрируем событие
     * обычного "клика", то при воспроизведении это приведет к закрытию QListView. Поэтому
     * возвращаем комментарий, так как в противном случае, если пользователь выберет элемент
     * из списка, то это может приведет к ошибке.
     */
    return QStringLiteral(
               "// 'Release' event is outside of QComboBox, so it is still opened\n// %1")
        .arg(qMouseEventFilter(utils::objectPath(comboBox), comboBox, event));
}

static QString qSliderFilter(const QWidget *widget, const QMouseEvent *event,
                             const ExtraInfoForDelayed &extra)
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::Slider));
    if (widget == nullptr) {
        return QString();
    }

    assert(extra.changeType.has_value());
    if (*extra.changeType == QAbstractSlider::SliderNoAction) {
        return QString();
    }

    auto *slider = qobject_cast<const QAbstractSlider *>(widget);
    assert(slider != nullptr);

    // Рассматриваем отдельно, так как любое зарегестрированное нажатие
    // на QDial приводит к установке значения "под курсором"
    if (qobject_cast<const QDial *>(widget)) {
        return utils::setValueStatement(widget, slider->value());
    }

    //! TODO: надо удостовериться, возможно ли вызвать SliderSingleStepAdd(Sub) нажатием мыши,
    //! если нет - то убрать эти типы из проверки
    switch (*extra.changeType) {
    case QAbstractSlider::SliderSingleStepAdd:
        return utils::changeValueStatement(widget, "SingleStepAdd");
    case QAbstractSlider::SliderSingleStepSub:
        return utils::changeValueStatement(widget, "SingleStepSub");
    case QAbstractSlider::SliderPageStepAdd:
        return utils::changeValueStatement(widget, "PageStepAdd");
    case QAbstractSlider::SliderPageStepSub:
        return utils::changeValueStatement(widget, "PageStepSub");
    case QAbstractSlider::SliderToMinimum:
        return utils::changeValueStatement(widget, "ToMinimum");
    case QAbstractSlider::SliderToMaximum:
        return utils::changeValueStatement(widget, "ToMaximum");
    case QAbstractSlider::SliderMove:
        return utils::setValueStatement(widget, slider->value());
    }
    Q_UNREACHABLE();
}

static QString qSpinBoxFilter(const QWidget *widget, const QMouseEvent *event,
                              const ExtraInfoForDelayed &extra) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::SpinBox));
    if (widget == nullptr) {
        return QString();
    }

    if (auto *dateEdit = qobject_cast<const QDateEdit *>(widget)) {
        return utils::setValueStatement(widget, dateEdit->date().toString(Qt::ISODate));
    }
    else if (auto *timeEdit = qobject_cast<const QTimeEdit *>(widget)) {
        return utils::setValueStatement(widget, timeEdit->time().toString(Qt::ISODate));
    }
    else if (auto *dateTimeEdit = qobject_cast<const QDateTimeEdit *>(widget)) {
        return utils::setValueStatement(widget, dateTimeEdit->dateTime().toString(Qt::ISODate));
    }

    bool isDblClick = event->type() == QEvent::MouseButtonDblClick;
    if (extra.isContinuous || isDblClick) {
        auto handleSpinBox = [&](auto *spinBox) {
            //! TODO: костыль, так как spinBoxWidget->value() при
            //! MouseButtonDblClick почему-то не соответствует действительности, что
            //! странно, так как значение изменяется при событии нажатия, а не
            //! отпускания, а на этапе обработки MouseButtonDblClick значение должно
            //! было измениться два раза
            return utils::setValueStatement(widget, spinBox->value()
                                                        + (isDblClick ? spinBox->singleStep() : 0));
        };
        if (auto *spinBox = qobject_cast<const QSpinBox *>(widget)) {
            return handleSpinBox(spinBox);
        }
        else if (auto *doubleSpinBox = qobject_cast<const QDoubleSpinBox *>(widget)) {
            return handleSpinBox(doubleSpinBox);
        }
        Q_UNREACHABLE();
    }
    else {
        const QRect upButtonRect(0, 0, widget->width(), widget->height() / 2);
        const QRect downButtonRect(0, widget->height() / 2, widget->width(), widget->height() / 2);

        if (upButtonRect.contains(event->pos())) {
            return utils::changeValueStatement(widget, "Up");
        }
        else if (downButtonRect.contains(event->pos())) {
            return utils::changeValueStatement(widget, "Down");
        }
    }

    return QString();
}

static QString qCalendarFilter(const QWidget *widget, const QMouseEvent *event,
                               const ExtraInfoForDelayed &extra) noexcept
{
    Q_UNUSED(extra);
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::Calendar));
    if (widget == nullptr) {
        return QString();
    }

    //! TODO:
    //! 1. QCalendarWidget не предоставляет возможности получить его модель данных,
    //! а она необходима, так как на этапе отпускания мыши значение меняется только в
    //! модели, но не в QCalendarWidget. Поэтому приходится работать с моделью напрямую,
    //! а сам QCalendarWidget получать через родителя модели, что не совсем хорошо...
    //!
    //! 2. Надо ли генерировать строку, если было произведено нажатие на уже выбранную
    //! дату?
    const auto *calendar = qobject_cast<const QCalendarWidget *>(widget->parentWidget());
    assert(calendar != nullptr);
    const auto *calendarView = qobject_cast<const QAbstractItemView *>(widget);
    assert(calendarView != nullptr);

    const auto currentCellIndex = calendarView->currentIndex();

    const auto selectedCellIndexes = calendarView->selectionModel()->selectedIndexes();
    assert(selectedCellIndexes.size() <= 1);
    const auto selectedCellIndex = selectedCellIndexes.first();
    const auto clickPos = calendarView->mapFromGlobal(event->globalPos());
    const auto dateChanged
        = calendarView->rect().contains(clickPos)
          && ((currentCellIndex != selectedCellIndex && event->type() == QEvent::MouseButtonRelease)
              || event->type() == QEvent::MouseButtonDblClick);

    assert(currentCellIndex.isValid());
    assert(currentCellIndex.data().canConvert<int>());

    const int day = currentCellIndex.data().toInt();
    int month = calendar->monthShown();
    int year = calendar->yearShown();

    QModelIndex repeatingDayIndex;
    auto *calendarModel = calendarView->model();
    for (int row = 0; row < calendarModel->rowCount(); ++row) {
        for (int column = 0; column < calendarModel->columnCount(); ++column) {
            QModelIndex index = calendarModel->index(row, column);
            assert(index.isValid());
            assert(index.data().canConvert<int>());
            if (index.data().toInt() == day && index != currentCellIndex) {
                repeatingDayIndex = index;
                break;
            }
        }

        if (repeatingDayIndex.isValid()) {
            break;
        }
    }

    if (repeatingDayIndex.isValid()) {
        if (repeatingDayIndex.row() < currentCellIndex.row()
            || (repeatingDayIndex.row() == currentCellIndex.row()
                && repeatingDayIndex.column() < currentCellIndex.column())) {
            month++;
            if (month > 12) {
                month = 1;
                year++;
            }
        }
        else {
            month--;
            if (month < 1) {
                month = 12;
                year--;
            }
        }
    }
    auto currentDate = calendar->calendar().dateFromParts(year, month, day);
    assert(currentDate.isValid());

    return QStringLiteral("%1%2")
        .arg(dateChanged ? "" : "// Looks like this date was not selected\n// ")
        .arg(utils::setValueStatement(calendar, currentDate.toString(Qt::ISODate)));
}

static QString qMenuFilter(const QWidget *widget, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::Menu));
    if (widget == nullptr) {
        return QString();
    }

    auto *menu = qobject_cast<const QMenu *>(widget);
    assert(menu != nullptr);

    const auto clickPos = widget->mapFromGlobal(event->globalPos());
    auto *action = menu->actionAt(clickPos);

    if (action == nullptr) {
        const auto menuText = menu->title();
        return QStringLiteral("activateMenu('%1')%2")
            .arg(utils::objectPath(widget))
            .arg(menuText.isEmpty() ? "" : QStringLiteral(" // Menu title: '%1'").arg(menuText));
    }
    else {
        const auto actionText = action->text();
        return QStringLiteral("%1activateMenuAction('%2', '%3'%4)%5")
            .arg(action->isSeparator() ? "// Looks like QMenu::Separator clicked\n// " : "")
            .arg(utils::objectPath(widget))
            .arg(utils::widgetIdInView(menu, menu->actions().indexOf(action), WidgetClass::Menu))
            .arg(action->isCheckable()
                     ? QStringLiteral(", %1").arg(action->isChecked() ? "false" : "true")
                     : "")
            .arg(actionText.isEmpty() ? ""
                                      : QStringLiteral(" // Action text: '%1'").arg(actionText));
    }
}

static QString qTabBarFilter(const QWidget *widget, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::TabBar));
    if (widget == nullptr) {
        return QString();
    }

    auto *tabBar = qobject_cast<const QTabBar *>(widget);
    assert(tabBar != nullptr);

    const auto currentIndex = tabBar->currentIndex();
    const auto currentText = tabBar->tabText(currentIndex);
    return QStringLiteral("selectTabItem('%1', '%2')%3")
        .arg(utils::objectPath(widget))
        .arg(utils::widgetIdInView(tabBar, currentIndex, WidgetClass::TabBar))
        .arg(currentText.isEmpty() ? ""
                                   : QStringLiteral(" // Tab item text: '%1'").arg(currentText));
}
} // namespace QtAda::core::filters

namespace QtAda::core {
WidgetEventFilter::WidgetEventFilter(QObject *parent) noexcept
    : QObject{ parent }
{
    filterFunctions_ = {
        filters::qRadioButtonFilter,
        filters::qCheckBoxFilter,
        filters::qComboBoxFilter,
        filters::qMenuFilter,
        filters::qTabBarFilter,
        // Обязательно в таком порядке:
        filters::qButtonFilter,
    };

    delayedFilterFunctions_ = {
        { WidgetClass::Slider, filters::qSliderFilter },
        { WidgetClass::SpinBox, filters::qSpinBoxFilter },
        { WidgetClass::Calendar, filters::qCalendarFilter },
    };
}

QString WidgetEventFilter::callWidgetFilters(const QWidget *widget, const QMouseEvent *event,
                                             bool isContinuous) noexcept
{
    const auto delayedResult = callDelayedFilter(widget, event, isContinuous);
    if (delayedResult.has_value() && !(*delayedResult).isEmpty()) {
        return *delayedResult;
    }

    QString result;
    for (auto &filter : filterFunctions_) {
        result = filter(widget, event);
        if (!result.isEmpty()) {
            return result;
        }
    }
    return result;
}

void WidgetEventFilter::findAndSetDelayedFilter(const QWidget *widget,
                                                const QMouseEvent *event) noexcept
{
    if (widget == delayedWidget_ && event == causedEvent_
        && causedEventType_ == QEvent::MouseButtonPress
        && event->type() == QEvent::MouseButtonDblClick) {
        return;
    }

    destroyDelay();
    WidgetClass foundWidgetClass = WidgetClass::None;
    QMetaObject::Connection connection;
    if (auto *foundWidget
        = utils::searchSpecificWidget(widget, filters::s_widgetMetaMap.at(WidgetClass::SpinBox))) {
        auto slot = [this] { emit this->signalDetected(); };
        foundWidgetClass = WidgetClass::SpinBox;
        connection = utils::connectIfType<QSpinBox>(
            widget, this, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), slot);
        if (!connection) {
            connection = utils::connectIfType<QDoubleSpinBox>(
                foundWidget, this,
                static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), slot);
        }
        if (!connection) {
            connection = utils::connectIfType<QDateTimeEdit>(
                foundWidget, this,
                static_cast<void (QDateTimeEdit::*)(const QDateTime &)>(
                    &QDateTimeEdit::dateTimeChanged),
                slot);
        }
    }
    else if (auto *foundWidget = utils::searchSpecificWidget(
                 widget, filters::s_widgetMetaMap.at(WidgetClass::Slider))) {
        auto slot = [this](int type) { emit this->specificSignalDetected(type); };
        foundWidgetClass = WidgetClass::Slider;
        connection = utils::connectIfType<QAbstractSlider>(
            foundWidget, this,
            static_cast<void (QAbstractSlider::*)(int)>(&QAbstractSlider::actionTriggered), slot);
    }
    else if (auto *foundWidget = utils::searchSpecificWidget(
                 widget, filters::s_widgetMetaMap.at(WidgetClass::Calendar))) {
        auto *calendarView = qobject_cast<const QAbstractItemView *>(foundWidget);
        assert(calendarView != nullptr);
        auto slot = [this] { emit this->signalDetected(); };
        foundWidgetClass = WidgetClass::Calendar;
        connection = utils::connectIfType<QItemSelectionModel>(
            calendarView->selectionModel(), this,
            static_cast<void (QItemSelectionModel::*)(const QModelIndex &, const QModelIndex &)>(
                &QItemSelectionModel::currentChanged),
            slot);
    }

    if (foundWidgetClass != WidgetClass::None) {
        assert(connection);
        initDelay(widget, event, delayedFilterFunctions_.at(foundWidgetClass), connection);
    }
}

bool WidgetEventFilter::delayedFilterCanBeCalledForWidget(const QWidget *widget) const noexcept
{
    return needToUseFilter_ && !connection_ && delayedFilter_.has_value()
           && delayedWidget_ != nullptr && delayedWidget_ == widget;
}

void WidgetEventFilter::initDelay(const QWidget *widget, const QMouseEvent *event,
                                  const DelayedWidgetFilterFunction &filter,
                                  QMetaObject::Connection &connection) noexcept
{
    causedEvent_ = event;
    causedEventType_ = event->type();
    delayedWidget_ = widget;
    delayedFilter_ = filter;
    connection_ = connection;
}

void WidgetEventFilter::destroyDelay() noexcept
{
    causedEventType_ = QEvent::None;
    delayedWidget_ = nullptr;
    delayedFilter_ = std::nullopt;
    delayedExtra_.clear();
    needToUseFilter_ = false;
    if (connection_) {
        QObject::disconnect(connection_);
    }
}

std::optional<QString> WidgetEventFilter::callDelayedFilter(const QWidget *widget,
                                                            const QMouseEvent *event,
                                                            bool isContinuous) noexcept
{
    delayedExtra_.isContinuous = isContinuous;
    bool callable = delayedFilterCanBeCalledForWidget(widget);
    return callable ? std::make_optional((*delayedFilter_)(widget, event, delayedExtra_))
                    : std::nullopt;
}
} // namespace QtAda::core
