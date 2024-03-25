#include "WidgetEventFilter.hpp"

#include <QString>
#include <QObject>
#include <QMouseEvent>
#include <map>

#include <QAbstractButton>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QListView>

#include "utils/Common.hpp"
#include "utils/FilterUtils.hpp"

//! TODO: remove
#include <iostream>

namespace QtAda::core::filters {
//! TODO: возможно, вектор - лишний
static const std::map<WidgetClass, QVector<QLatin1String>> s_widgetMetaMap = {
    { Button, { QLatin1String("QAbstractButton") } },
    { CheckBox, { QLatin1String("QCheckBox") } },
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
// Для QButton, QRadioButton
static QString qButtonFilter(const QWidget *widget, const QMouseEvent *event, bool) noexcept
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

    const auto buttonText = button->text();

    if (buttonText.isEmpty()) {
        return QStringLiteral("clickButton('%1')").arg(utils::objectPath(widget));
    }
    else {
        return QStringLiteral("clickButton('%1') // Button text: '%2'")
            .arg(utils::objectPath(widget))
            .arg(buttonText);
    }
}

static QString qCheckBoxFilter(const QWidget *widget, const QMouseEvent *event, bool) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::CheckBox));
    if (widget == nullptr) {
        return QString();
    }

    auto *checkBox = qobject_cast<const QCheckBox *>(widget);
    assert(checkBox != nullptr);

    return QStringLiteral("checkButton('%1', %2) // Button text: '%3'")
        .arg(utils::objectPath(widget))
        .arg(checkBox->isChecked() ? "false" : "true")
        .arg(checkBox->text());
}

static QString qComboBoxFilter(const QWidget *widget, const QMouseEvent *event, bool) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    auto [comboBox, iteration] = utils::searchSpecificWidgetWithIteration(
        widget, s_widgetMetaMap.at(WidgetClass::ComboBox), 4);
    if (comboBox == nullptr) {
        return QString();
    }
    if (iteration == 2) {
        return QStringLiteral("// Looks like QComboBox expansion\n// %1")
            .arg(qMouseEventFilter(utils::objectPath(comboBox), comboBox, event));
    }

    auto *comboBoxView = qobject_cast<const QListView *>(widget);
    if (comboBoxView == nullptr && widget->parent() != nullptr) {
        comboBoxView = qobject_cast<const QListView *>(widget->parent());
    }
    assert(comboBoxView != nullptr);

    return QStringLiteral("selectItem('%1', '%2')")
        .arg(utils::objectPath(comboBox))
        .arg(utils::itemIdInWidgetView(comboBox, comboBoxView->currentIndex(),
                                       WidgetClass::ComboBox));
}

static QString qSpinBoxFilter(const QWidget *widget, const QMouseEvent *event,
                              bool isContinuous) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::SpinBox));
    if (widget == nullptr) {
        return QString();
    }

    if (auto *dateEdit = qobject_cast<const QDateEdit *>(widget)) {
        return utils::setValueStatement(utils::objectPath(widget),
                                        dateEdit->date().toString(Qt::ISODate), true);
    }
    else if (auto *timeEdit = qobject_cast<const QTimeEdit *>(widget)) {
        return utils::setValueStatement(utils::objectPath(widget),
                                        timeEdit->time().toString(Qt::ISODate), true);
    }
    else if (auto *dateTimeEdit = qobject_cast<const QDateTimeEdit *>(widget)) {
        return utils::setValueStatement(utils::objectPath(widget),
                                        dateTimeEdit->dateTime().toString(Qt::ISODate), true);
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
                utils::objectPath(widget),
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
            return QStringLiteral("changeValue('%1', 'Up')").arg(utils::objectPath(widget));
        }
        else if (downButtonRect.contains(event->pos())) {
            return QStringLiteral("changeValue('%1', 'Down')").arg(utils::objectPath(widget));
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
        filters::qComboBoxFilter,
        filters::qCheckBoxFilter,
        // Обязательно последним
        filters::qButtonFilter,
    };

    delayedFilterFunctions_ = {
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
        result = filter(widget, event, isDelayed);
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
    auto slot = [this] { emit this->signalDetected(); };
    if (utils::searchSpecificWidget(widget, filters::s_widgetMetaMap.at(WidgetClass::SpinBox))
        != nullptr) {
        auto connection = utils::connectIfType<QSpinBox>(
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

        initDelay(widget, event, delayedFilterFunctions_.at(WidgetClass::SpinBox), connection);
    }
}

bool WidgetEventFilter::delayedFilterCanBeCalledForWidget(const QWidget *widget) const noexcept
{
    return needToUseFilter_ && !connection_ && delayedFilter_.has_value()
           && delayedWidget_ != nullptr && delayedWidget_ == widget;
}

void WidgetEventFilter::initDelay(const QWidget *widget, const QMouseEvent *event,
                                  const WidgetFilterFunction &filter,
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
    return callable ? std::make_optional((*delayedFilter_)(widget, event, isContinuous))
                    : std::nullopt;
}
} // namespace QtAda::core
