#include "WidgetEventFilters.hpp"

#include <QString>
#include <QObject>
#include <QMouseEvent>

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

QString qSpinBoxFilter(QWidget *widget, QMouseEvent *event, bool isDelayed)
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    auto *spinBox = utils::searchSpecificWidget(widget, utils::WidgetClass::SpinBox);
    if (spinBox == nullptr) {
        return QString();
    }

    if (isDelayed) {
        auto *spinBoxWidget = qobject_cast<QSpinBox *>(spinBox);
        if (spinBoxWidget != nullptr) {
            return QStringLiteral("setValue('%1', %2)")
                .arg(utils::objectPath(spinBox))
                .arg(spinBoxWidget->value());
        }

        auto *doubleSpinBoxWidget = qobject_cast<QDoubleSpinBox *>(spinBox);
        assert(doubleSpinBoxWidget != nullptr);
        return QStringLiteral("setValue('%1', %2)")
            .arg(utils::objectPath(spinBox))
            .arg(spinBoxWidget->value());
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
} // namespace QtAda::core
