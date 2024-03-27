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

#include "utils/Common.hpp"
#include "utils/FilterUtils.hpp"

//! TODO: remove
#include <iostream>

namespace QtAda::core::filters {
//! TODO: возможно, вектор - лишний
static const std::map<WidgetClass, QVector<QLatin1String>> s_widgetMetaMap = {
    { Button, { QLatin1String("QAbstractButton") } },
    { RadioButton, { QLatin1String("QRadioButton") } },
    { CheckBox, { QLatin1String("QCheckBox") } },
    { Slider, { QLatin1String("QAbstractSlider") } },
    { ComboBox, { QLatin1String("QComboBox") } },
    { SpinBox, { QLatin1String("QAbstractSpinBox") } },
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
        widget, s_widgetMetaMap.at(WidgetClass::ComboBox), 4);
    if (widget == nullptr) {
        return QString();
    }
    if (iteration <= 2) {
        return QStringLiteral("// Looks like QComboBox expansion\n// %1")
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
            .arg(utils::itemIdInWidgetView(comboBox, comboBoxView->currentIndex(),
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

static QString qSliderFilter(const QWidget *widget, const QMouseEvent *event, bool isContinuous,
                             std::optional<int> actionType)
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::Slider));
    if (widget == nullptr) {
        return QString();
    }

    assert(actionType.has_value());
    if (*actionType == QAbstractSlider::SliderNoAction) {
        return QString();
    }

    auto *slider = qobject_cast<const QAbstractSlider *>(widget);
    assert(slider != nullptr);

    // Рассматриваем отдельно, так как любое зарегестрированное нажатие
    // на QDial приводит к установке значения "под курсором"
    if (qobject_cast<const QDial *>(widget)) {
        return utils::setValueStatement(widget, QString::number(slider->value()));
    }

    //! TODO: надо удостовериться, возможно ли вызвать SliderSingleStepAdd(Sub) нажатием мыши,
    //! если нет - то убрать эти типы из проверки
    switch (*actionType) {
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
        return utils::setValueStatement(widget, QString::number(slider->value()));
    }
    Q_UNREACHABLE();
}

static QString qSpinBoxFilter(const QWidget *widget, const QMouseEvent *event, bool isContinuous,
                              std::optional<int>) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::SpinBox));
    if (widget == nullptr) {
        return QString();
    }

    if (auto *dateEdit = qobject_cast<const QDateEdit *>(widget)) {
        return utils::setValueStatement(widget, dateEdit->date().toString(Qt::ISODate), true);
    }
    else if (auto *timeEdit = qobject_cast<const QTimeEdit *>(widget)) {
        return utils::setValueStatement(widget, timeEdit->time().toString(Qt::ISODate), true);
    }
    else if (auto *dateTimeEdit = qobject_cast<const QDateTimeEdit *>(widget)) {
        return utils::setValueStatement(widget, dateTimeEdit->dateTime().toString(Qt::ISODate),
                                        true);
    }

    bool isDblClick = event->type() == QEvent::MouseButtonDblClick;
    if (isContinuous || isDblClick) {
        auto handleSpinBox = [&](auto *spinBox) {
            //! TODO: костыль, так как spinBoxWidget->value() при
            //! MouseButtonDblClick почему-то не соответствует действительности, что
            //! странно, так как значение изменяется при событии нажатия, а не
            //! отпускания, а на этапе обработки MouseButtonDblClick значение должно
            //! было измениться два раза
            return utils::setValueStatement(
                widget,
                QString::number(spinBox->value() + (isDblClick ? spinBox->singleStep() : 0)));
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
} // namespace QtAda::core::filters

namespace QtAda::core {
WidgetEventFilter::WidgetEventFilter(QObject *parent) noexcept
    : QObject{ parent }
{
    filterFunctions_ = {
        filters::qRadioButtonFilter,
        filters::qCheckBoxFilter,
        filters::qComboBoxFilter,
        // Обязательно последним
        filters::qButtonFilter,
    };

    delayedFilterFunctions_ = {
        { WidgetClass::Slider, filters::qSliderFilter },
        { WidgetClass::SpinBox, filters::qSpinBoxFilter },
    };
}

QString WidgetEventFilter::callWidgetFilters(const QWidget *widget, const QMouseEvent *event,
                                             bool isDelayed) const noexcept
{
    const auto delayedResult = callDelayedFilter(widget, event, isDelayed);
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

    if (utils::searchSpecificWidget(widget, filters::s_widgetMetaMap.at(WidgetClass::SpinBox))
        != nullptr) {
        auto slot = [this] { emit this->signalDetected(); };
        foundWidgetClass = WidgetClass::SpinBox;
        connection = utils::connectIfType<QSpinBox>(
            widget, this, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), slot);
        if (!connection) {
            connection = utils::connectIfType<QDoubleSpinBox>(
                widget, this,
                static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), slot);
        }
        if (!connection) {
            connection = utils::connectIfType<QDateTimeEdit>(
                widget, this,
                static_cast<void (QDateTimeEdit::*)(const QDateTime &)>(
                    &QDateTimeEdit::dateTimeChanged),
                slot);
        }
    }
    else if (utils::searchSpecificWidget(widget, filters::s_widgetMetaMap.at(WidgetClass::Slider))
             != nullptr) {
        auto slot = [this](int type) { emit this->specificSignalDetected(type); };
        foundWidgetClass = WidgetClass::Slider;
        connection = utils::connectIfType<QAbstractSlider>(
            widget, this,
            static_cast<void (QAbstractSlider::*)(int)>(&QAbstractSlider::actionTriggered), slot);
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
    delayedActionType_ = std::nullopt;
    needToUseFilter_ = false;
    if (connection_) {
        QObject::disconnect(connection_);
    }
}

std::optional<QString> WidgetEventFilter::callDelayedFilter(const QWidget *widget,
                                                            const QMouseEvent *event,
                                                            bool isContinuous) const noexcept
{
    bool callable = delayedFilterCanBeCalledForWidget(widget);
    return callable ? std::make_optional(
               (*delayedFilter_)(widget, event, isContinuous, delayedActionType_))
                    : std::nullopt;
}
} // namespace QtAda::core
