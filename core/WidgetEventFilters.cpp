#include "WidgetEventFilters.hpp"

#include <QString>
#include <QObject>
#include <QMouseEvent>
#include <map>

#include <QComboBox>
#include <QListView>
#include <QSpinBox>
#include <QCheckBox>
#include <QAbstractButton>

#include "utils/FilterUtils.hpp"

//! TODO: remove
#include <iostream>

namespace QtAda::core {
QString qMouseEventFilter(const QString &path, QWidget *widget, QMouseEvent *event)
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

QString qComboBoxFilter(QWidget *widget, QMouseEvent *event, bool)
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    const auto widgetClass = utils::WidgetClass::ComboBox;
    auto [comboBox, iteration] = utils::searchSpecificWidgetWithIteration(widget, widgetClass);
    if (comboBox == nullptr) {
        return QString();
    }
    if (iteration == 1) {
        return QStringLiteral("// Looks like QComboBox expansion\n// %1")
            .arg(qMouseEventFilter(utils::objectPath(comboBox), comboBox, event));
    }

    auto *comboBoxView = qobject_cast<QListView *>(widget);
    if (comboBoxView == nullptr && widget->parent() != nullptr) {
        comboBoxView = qobject_cast<QListView *>(widget->parent());
    }
    if (comboBoxView == nullptr) {
        //! TODO: может все-таки надо возвращать ошибку в этом случае
        return QString();
    }

    const auto position = widget->mapFromGlobal(event->globalPos());
    const auto itemIndex = comboBoxView->indexAt(position);

    return QStringLiteral("selectItem('%1', '%2')")
        .arg(utils::objectPath(comboBox))
        .arg(itemIdInWidgetView(comboBox, itemIndex, widgetClass));
}

QString qCheckBoxFilter(QWidget *widget, QMouseEvent *event, bool)
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    auto *checkBox = utils::searchSpecificWidget(widget, utils::WidgetClass::CheckBox);
    if (checkBox == nullptr) {
        return QString();
    }

    auto *checkBoxWidget = qobject_cast<QCheckBox *>(checkBox);
    assert(checkBoxWidget != nullptr);

    const auto releasePosition = checkBoxWidget->mapFromGlobal(event->globalPos());
    if (checkBoxWidget->rect().contains(releasePosition)) {
        return QStringLiteral("checkButton('%1', %2) // Button text: '%3'")
            .arg(utils::objectPath(checkBox))
            .arg(checkBoxWidget->isChecked() ? "false" : "true")
            .arg(checkBoxWidget->text());
    }
    else {
        return qMouseEventFilter(utils::objectPath(checkBox), checkBox, event);
    }
}

static QString qSpinBoxFilter(QWidget *widget, QMouseEvent *event, bool isContinuous)
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    auto *spinBox = utils::searchSpecificWidget(widget, utils::WidgetClass::SpinBox);
    if (spinBox == nullptr) {
        return QString();
    }

    bool isDblClick = event->type() == QEvent::MouseButtonDblClick;
    if (isContinuous || isDblClick) {
        auto *spinBoxWidget = qobject_cast<QSpinBox *>(spinBox);
        if (spinBoxWidget != nullptr) {
            return QStringLiteral("setValue('%1', %2)")
                .arg(utils::objectPath(spinBox))
                //! TODO: костыль, так как spinBoxWidget->value() при
                //! MouseButtonDblClick почему-то не соответствует действительности, что
                //! странно, так как значение изменяется при событии нажатия, а не
                //! отпускания, а на этапе обработки MouseButtonDblClick значение должно
                //! было измениться два раза
                .arg(spinBoxWidget->value() + (isDblClick ? spinBoxWidget->singleStep() : 0));
        }

        auto *doubleSpinBoxWidget = qobject_cast<QDoubleSpinBox *>(spinBox);
        assert(doubleSpinBoxWidget != nullptr);
        return QStringLiteral("setValue('%1', %2)")
            .arg(utils::objectPath(spinBox))
            //! TODO: костыль (см. выше)
            .arg(spinBoxWidget->value() + (isDblClick ? spinBoxWidget->singleStep() : 0));
    }
    else {
        const QRect upButtonRect(0, 0, spinBox->width(), spinBox->height() / 2);
        const QRect downButtonRect(0, spinBox->height() / 2, spinBox->width(),
                                   spinBox->height() / 2);

        if (upButtonRect.contains(event->pos())) {
            return QStringLiteral("changeValue('%1', 'Up')").arg(utils::objectPath(spinBox));
        }
        else if (downButtonRect.contains(event->pos())) {
            return QStringLiteral("changeValue('%1', 'Down')").arg(utils::objectPath(spinBox));
        }
    }

    return QString();
}

//! TODO: нужна ли обработка зажатия кастомной кнопки?
// Для QButton, QRadioButton
QString qButtonFilter(QWidget *widget, QMouseEvent *event, bool)
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    //! TODO: скорее всего нужно будет уточнять какие именно классы, а не просто QAbstractButton
    auto *button = utils::searchSpecificWidget(widget, utils::WidgetClass::Button);
    if (button == nullptr) {
        return QString();
    }

    auto *abstractButton = qobject_cast<QAbstractButton *>(button);
    assert(abstractButton != nullptr);

    const auto buttonText = abstractButton->text();

    if (buttonText.isEmpty()) {
        return QStringLiteral("clickButton('%1')").arg(utils::objectPath(button));
    }
    else {
        return QStringLiteral("clickButton('%1') // Button text: '%2'")
            .arg(utils::objectPath(button))
            .arg(buttonText);
    }
}

static const std::map<utils::WidgetClass, WidgetEventFilter> s_delayedFilters = {
    { utils::WidgetClass::SpinBox, qSpinBoxFilter },
};

void DelayedWidgetFilter::findAndSetDelayedFilter(QWidget *widget, QMouseEvent *event) noexcept
{
    if (widget == delayedWidget_ && event == causedEvent_
        && causedEventType_ == QEvent::MouseButtonPress
        && event->type() == QEvent::MouseButtonDblClick) {
        return;
    }

    destroyDelay();
    if (utils::searchSpecificWidget(widget, utils::WidgetClass::SpinBox, 1) != nullptr) {
        auto *spinBox = qobject_cast<QSpinBox *>(widget);
        assert(spinBox != nullptr);
        QMetaObject::Connection connection = connect(
            spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this] {
                std::cout << "ValueChanged" << std::endl;
                emit this->signalDetected();
            });
        setDelayedFilter(widget, event, s_delayedFilters.at(utils::WidgetClass::SpinBox),
                         connection);
    }
}

bool DelayedWidgetFilter::delayedFilterCanBeCalledForWidget(const QWidget *widget) const noexcept
{
    return needToUseFilter_ && !connection_ && delayedFilter_.has_value()
           && delayedWidget_ != nullptr && delayedWidget_ == widget;
}

void DelayedWidgetFilter::setDelayedFilter(QWidget *widget, QMouseEvent *event,
                                           const WidgetEventFilter &filter,
                                           QMetaObject::Connection &connection) noexcept
{
    causedEvent_ = event;
    causedEventType_ = event->type();
    delayedWidget_ = widget;
    delayedFilter_ = filter;
    connection_ = connection;
}

void DelayedWidgetFilter::destroyDelay() noexcept
{
    causedEventType_ = QEvent::None;
    delayedWidget_ = nullptr;
    delayedFilter_ = std::nullopt;
    needToUseFilter_ = false;
    if (connection_) {
        QObject::disconnect(connection_);
    }
}

std::optional<QString> DelayedWidgetFilter::callDelayedFilter(QWidget *widget, QMouseEvent *event,
                                                              bool isContinuous) noexcept
{
    bool callable = delayedFilterCanBeCalledForWidget(widget);
    return callable ? std::make_optional((*delayedFilter_)(widget, event, isContinuous))
                    : std::nullopt;
}
} // namespace QtAda::core
