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
#include <QMenuBar>
#include <QAction>
#include <QTabBar>
#include <QTreeView>
#include <QUndoView>
#include <QUndoStack>
#include <QDialog>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QKeySequenceEdit>

#include "utils/CommonFilterUtils.hpp"
#include "utils/WidgetFilterUtils.hpp"

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
    { WidgetClass::Button, { QLatin1String("QAbstractButton"), 1 } },
    { WidgetClass::RadioButton, { QLatin1String("QRadioButton"), 1 } },
    { WidgetClass::CheckBox, { QLatin1String("QCheckBox"), 1 } },
    { WidgetClass::Slider, { QLatin1String("QAbstractSlider"), 1 } },
    { WidgetClass::ComboBox, { QLatin1String("QComboBox"), 4 } },
    { WidgetClass::SpinBox, { QLatin1String("QAbstractSpinBox"), 1 } },
    { WidgetClass::Menu, { QLatin1String("QMenu"), 1 } },
    { WidgetClass::MenuBar, { QLatin1String("QMenuBar"), 1 } },
    { WidgetClass::TabBar, { QLatin1String("QTabBar"), 1 } },
    { WidgetClass::ItemView, { QLatin1String("QAbstractItemView"), 3 } },
    { WidgetClass::TreeView, { QLatin1String("QTreeView"), 2 } },
    { WidgetClass::UndoView, { QLatin1String("QUndoView"), 2 } },
    { WidgetClass::Calendar, { QLatin1String("QCalendarView"), 2 } },
    //! TODO: в официальной документации по 5.15 ни слова об этом классе,
    //! однако он в составе QColumnView. Если его не обрабатывать, то
    //! для ItemView генерируются неправильные события.
    { WidgetClass::ColumnViewGrip, { QLatin1String("QColumnViewGrip"), 1 } },
    { WidgetClass::Dialog, { QLatin1String("QDialog"), 1 } },
    { WidgetClass::Window, { QLatin1String("QMainWindow"), 1 } },
    { WidgetClass::KeySequenceEdit, { QLatin1String("QKeySequenceEdit"), 1 } },
    { WidgetClass::TextEdit, { QLatin1String("QTextEdit"), 1 } },
    { WidgetClass::PlainTextEdit, { QLatin1String("QPlainTextEdit"), 1 } },
    { WidgetClass::LineEdit, { QLatin1String("QLineEdit"), 1 } },
};

static const std::vector<WidgetClass> s_processedTextWidgets = {
    WidgetClass::TextEdit,        WidgetClass::PlainTextEdit, WidgetClass::LineEdit,
    WidgetClass::KeySequenceEdit, WidgetClass::ComboBox,      WidgetClass::SpinBox,
};

QString qMouseEventFilter(const QWidget *widget, const QEvent *event, const QString &path) noexcept
{
    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (widget == nullptr || mouseEvent == nullptr) {
        return QString();
    }

    const auto clickPosition = widget->mapFromGlobal(mouseEvent->globalPos());
    return QStringLiteral("mouse%1Click('%2', '%3', %4, %5);")
        .arg(event->type() == QEvent::MouseButtonDblClick ? "Dbl" : "")
        .arg(path.isEmpty() ? utils::objectPath(widget) : path)
        .arg(utils::mouseButtonToString(mouseEvent->button()))
        .arg(clickPosition.x())
        .arg(clickPosition.y());
}

QString qKeyEventFilter(const QWidget *widget, const QEvent *event, const QString &path) noexcept
{
    auto *keyEvent = static_cast<const QKeyEvent *>(event);
    if (widget == nullptr || keyEvent == nullptr) {
        return QString();
    }

    return QStringLiteral("%1('%2', '%3');")
        .arg("keyEvent")
        .arg(path.isEmpty() ? utils::objectPath(widget) : path)
        .arg(utils::escapeText(keyEvent->text()));
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

    return QStringLiteral("%1Button('%2');%3")
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
            return QStringLiteral("clickButton('%1');%2")
                .arg(utils::objectPath(widget))
                .arg(radioButton->text().isEmpty()
                         ? ""
                         : QStringLiteral(" // Button text: '%1'").arg(radioButton->text()));
        }
    }
    return qMouseEventFilter(widget, event);
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
            return QStringLiteral("checkButton('%1', %2);%3")
                .arg(utils::objectPath(widget))
                .arg(checkBox->isChecked() ? "false" : "true")
                .arg(checkBox->text().isEmpty()
                         ? ""
                         : QStringLiteral(" // Button text: '%1'").arg(checkBox->text()));
            ;
        }
    }
    return qMouseEventFilter(widget, event);
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
            .arg(qMouseEventFilter(widget, event));
    }

    auto *comboBox = qobject_cast<const QComboBox *>(widget);
    assert(comboBox != nullptr);

    //! TODO: нужно проверить, если выполнение кода дошло до этого места, то точно ли
    //! был нажат элемент списка
    auto *comboBoxView = comboBox->view();
    const auto containerRect = comboBoxView->rect();
    const auto clickPos = comboBoxView->mapFromGlobal(event->globalPos());

    if (containerRect.contains(clickPos)) {
        return QStringLiteral("selectItem('%1', '%2');")
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
        .arg(qMouseEventFilter(comboBox, event));
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

static QString qTreeViewFilter(const QWidget *widget, const QMouseEvent *event,
                               const ExtraInfoForDelayed &extra) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::TreeView));
    if (widget == nullptr) {
        return QString();
    }

    // В этом фильтре обрабатываем только Expanded и Collapsed события для QTreeView,
    // для остальных событий будет вызван фильтр qItemViewFilter
    assert(extra.changeType.has_value());
    assert(*extra.changeType == ExtraInfoForDelayed::TreeViewExtra::Collapsed
           || *extra.changeType == ExtraInfoForDelayed::TreeViewExtra::Expanded);
    assert(extra.changeIndex.isValid());

    auto *view = qobject_cast<const QAbstractItemView *>(widget);
    assert(view != nullptr);

    const auto currentItem = view->model()->data(extra.changeIndex);
    const auto currentItemText
        = currentItem.canConvert<QString>() ? currentItem.toString() : QString();
    return QStringLiteral("%1Delegate('%2');%3")
        .arg(extra.changeType == ExtraInfoForDelayed::TreeViewExtra::Expanded ? "expand"
                                                                              : "collapse")
        .arg(utils::objectPath(widget))
        .arg(currentItemText.isEmpty()
                 ? ""
                 : QStringLiteral(" // Delegate text: '%1'").arg(currentItemText));
}

static QString qUndoViewFilter(const QWidget *widget, const QMouseEvent *event,
                               const ExtraInfoForDelayed &extra) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event) || extra.collectedIndexes.empty()) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::ItemView));
    if (widget == nullptr) {
        return QString();
    }

    auto *view = qobject_cast<const QAbstractItemView *>(widget);
    assert(view != nullptr);
    const auto *model = view->model();
    assert(model != nullptr);

    QString result;
    for (const auto &index : extra.collectedIndexes) {
        if (!result.isEmpty()) {
            result += '\n';
        }

        const auto currentItem = model->data(model->index(index, 0));
        const auto currentItemText
            = currentItem.canConvert<QString>() ? currentItem.toString() : QString();
        result += QStringLiteral("undoCommand('%1', %2);%3")
                      .arg(utils::objectPath(view))
                      .arg(index)
                      .arg(currentItemText.isEmpty()
                               ? ""
                               : QStringLiteral(" // Delegate text: '%1'").arg(currentItemText));
    }
    assert(!result.isEmpty());
    return result;
}

// Это вспомогательная функция для двух используемых фильтров: qItemViewFilter и
// qItemViewSelectFilter
static QString qItemViewClickFilter(const QAbstractItemView *view,
                                    const QMouseEvent *event) noexcept
{
    const auto *selectionModel = view->selectionModel();
    //! TODO: возможна ли ситуация `selectionModel == nullptr`?
    assert(selectionModel != nullptr);
    const auto currentIndex = view->currentIndex();
    const auto selectedIndexes = selectionModel->selectedIndexes();
    const auto selectedIndex
        = selectedIndexes.size() == 1 ? selectedIndexes.first() : QModelIndex();
    const auto clickedIndex = view->indexAt(event->pos());

    if ((view->selectionMode() == QAbstractItemView::NoSelection || selectedIndex == currentIndex)
        && currentIndex.isValid() && clickedIndex.isValid()) {
        const auto currentItem = view->model()->data(currentIndex);
        const auto currentItemText
            = currentItem.canConvert<QString>() ? currentItem.toString() : QString();
        return QStringLiteral("delegate%1Click('%2', (%3, %4));%5")
            .arg(event->type() == QEvent::MouseButtonDblClick ? "Dbl" : "")
            .arg(utils::objectPath(view))
            .arg(currentIndex.row())
            .arg(currentIndex.column())
            .arg(currentItemText.isEmpty()
                     ? ""
                     : QStringLiteral(" // Delegate text: '%1'").arg(currentItemText));
    }

    return QString();
}

static QString qItemViewFilter(const QWidget *widget, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    if (utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::ColumnViewGrip))
        != nullptr) {
        // Никак дополнительно не обрабатываем это действие, так как оно не влияет
        // на функционал, а влияет только на визуальное отображение элементов
        return QLatin1String("// Looks like QColumnViewGrip moved");
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::ItemView));
    if (widget == nullptr) {
        return QString();
    }

    /*
     * События для QUndoView обрабатываем отдельно, но если индекс в QUndoView, то никакого
     * полезного события и не будет, следовательно, обработчик дойдет до сюда, поэтому необходимо
     * отметить, что данный клик бесполезен.
     */
    bool isUndoView
        = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::UndoView)) != nullptr;

    auto *view = qobject_cast<const QAbstractItemView *>(widget);
    assert(view != nullptr);

    //! TODO:
    //! 1. Основная проблема в том, что пока что нет четкого понимания, как работать с моделями.
    //! Чаще бывает так, что данные в модели - далеко не постоянная величина. Также часто бывает,
    //! что эти данные нельзя представить в виде текста. Поэтому на текущий момент было принято
    //! решение обращаться к этим данным только по индексу.
    //!
    //! 2. Также не очень понятно, обработка каких действий может быть в принципе полезна для
    //! тестирования. Основное обрабатываемое действие - `клик` по элементу, потому что обычно
    //! именно это событие и используется в тестируемых приложениях. На всякий случай мы
    //! пока что также обрабатываем `выбор` элементов, но скорее всего это будет лишним.

    const auto clickResult = qItemViewClickFilter(view, event);
    if (clickResult.isEmpty()) {
        const auto selectionMode = view->selectionMode();
        if (selectionMode == QAbstractItemView::ExtendedSelection
            || selectionMode == QAbstractItemView::ContiguousSelection) {
            return QStringLiteral("clearSelection('%1');").arg(utils::objectPath(view));
        }
        return QString();
    }

    return QStringLiteral("%1%2")
        .arg(isUndoView ? "// Looks like QUndoView useless delegate click\n// " : "")
        .arg(clickResult);
}

static QString qItemViewSelectionFilter(const QWidget *widget, const QMouseEvent *event,
                                        const ExtraInfoForDelayed &extra) noexcept
{
    Q_UNUSED(extra);
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::ItemView));
    if (widget == nullptr) {
        return QString();
    }

    auto *view = qobject_cast<const QAbstractItemView *>(widget);
    assert(view != nullptr);

    const auto clickResult = qItemViewClickFilter(view, event);
    if (!clickResult.isEmpty()) {
        return clickResult;
    }

    const auto selectedCellsData = utils::selectedCellsData(view->selectionModel());
    //! TODO: на этапе обработки записанных действий скорее всего придется переделать
    //! запись выбранных ячеек
    return selectedCellsData.isEmpty()
               ? QStringLiteral("clearSelection('%1');").arg(utils::objectPath(widget))
               : QStringLiteral("let selectionData = [%1];\nsetSelection('%2', selectionData);")
                     .arg(selectedCellsData)
                     .arg(utils::objectPath(widget));
}

static QString qMenuBarFilter(const QWidget *widget, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event, true)) {
        return QString();
    }

    widget = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::MenuBar));
    if (widget == nullptr) {
        return QString();
    }

    auto *menuBar = qobject_cast<const QMenuBar *>(widget);
    assert(menuBar != nullptr);

    const auto clickPos = menuBar->mapFromGlobal(event->globalPos());
    auto *action = menuBar->actionAt(clickPos);
    if (action == nullptr) {
        return QString();
    }

    const auto *actionMenu = action->menu();
    if (actionMenu == nullptr) {
        const auto actionText = action->text();
        return QStringLiteral("%1activateMenuAction('%2', '%3'%4);%5")
            .arg(action->isSeparator() ? " // Looks like QMenu::Separator clicked\n// " : "")
            .arg(utils::objectPath(widget))
            .arg(utils::widgetIdInView(menuBar, menuBar->actions().indexOf(action),
                                       WidgetClass::MenuBar))
            .arg(action->isCheckable()
                     ? QStringLiteral(", %1").arg(action->isChecked() ? "false" : "true")
                     : "")
            .arg(actionText.isEmpty() ? ""
                                      : QStringLiteral(" // Action text: '%1'").arg(actionText));
    }
    else {
        //! TODO: на текущий момент не обрабатывается DoubleClick по QMenu (непонятно почему), но
        //! нужно ли оно?
        const auto menuText = actionMenu->title();
        return QStringLiteral("activateMenu('%1');%2")
            .arg(utils::objectPath(widget))
            .arg(menuText.isEmpty() ? "" : QStringLiteral(" // Menu title: '%1'").arg(menuText));
    }
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
        return QStringLiteral("activateMenu('%1');%2")
            .arg(utils::objectPath(widget))
            .arg(menuText.isEmpty() ? "" : QStringLiteral(" // Menu title: '%1'").arg(menuText));
    }
    else {
        const auto actionText = action->text();
        return QStringLiteral("%1activateMenuAction('%2', '%3'%4);%5")
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
    return QStringLiteral("selectTabItem('%1', '%2');%3")
        .arg(utils::objectPath(widget))
        .arg(utils::widgetIdInView(tabBar, currentIndex, WidgetClass::TabBar))
        .arg(currentText.isEmpty() ? ""
                                   : QStringLiteral(" // Tab item text: '%1'").arg(currentText));
}

static QString qCloseFilter(const QWidget *widget, const QEvent *event) noexcept
{
    // Здесь обрабатываем только закрытие какого-либо окна приложения
    if (widget == nullptr || event == nullptr
        || (event != nullptr && event->type() != QEvent::Close)) {
        return QString();
    }

    if (utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::Dialog)) != nullptr) {
        return QStringLiteral("closeDialog(%1);").arg(utils::objectPath(widget));
    }
    if (utils::searchSpecificWidget(widget, s_widgetMetaMap.at(WidgetClass::Window)) != nullptr) {
        return QStringLiteral("closeWindow(%1);").arg(utils::objectPath(widget));
    }

    return QStringLiteral("// Looks like this QEvent::Close is not important\nclose(%1);")
        .arg(utils::objectPath(widget));
}

static QString qTextFocusFilters(const QWidget *widget, const QMouseEvent *event) noexcept
{
    for (const auto &widgetClass : s_processedTextWidgets) {
        if (auto *foundWidget
            = utils::searchSpecificWidget(widget, s_widgetMetaMap.at(widgetClass))) {
            QString widgetClassStr;
            switch (widgetClass) {
            case WidgetClass::TextEdit:
                widgetClassStr = QLatin1String("QTextEdit");
                break;
            case WidgetClass::PlainTextEdit:
                widgetClassStr = QLatin1String("QPlainTextEdit");
                break;
            case WidgetClass::LineEdit:
                widgetClassStr = QLatin1String("QLineEdit");
                break;
            case WidgetClass::SpinBox:
                widgetClassStr = QLatin1String("QSpinBox");
                break;
            default:
                // Для QComboBox есть собственный обработчик нажатия, который
                // должен быть вызван раньше текущего
                Q_UNREACHABLE();
            }

            const auto clickPos = foundWidget->mapFromGlobal(event->globalPos());
            return QStringLiteral("// Looks like focus click on %1\n// %2")
                .arg(widgetClassStr)
                .arg(qMouseEventFilter(foundWidget, event));
        }
    }
    return QString();
}

} // namespace QtAda::core::filters

namespace QtAda::core {
WidgetEventFilter::WidgetEventFilter(QObject *parent) noexcept
    : QObject{ parent }
{
    widgetMouseFilters_ = {
        filters::qRadioButtonFilter,
        filters::qCheckBoxFilter,
        filters::qComboBoxFilter,
        filters::qMenuFilter,
        filters::qTabBarFilter,
        filters::qItemViewFilter,
        // Обязательно в таком порядке:
        filters::qButtonFilter,
        filters::qTextFocusFilters,
    };

    //! TODO: Очень некрасивое решение. Но пока это работает для решения следующих проблем:
    //! 1. Мы не можем решать произведено было нажатие на QMenu или на QAction, так как
    //! при событии нажатия на "кнопку" QMenuBar (раскрытие списка), может быть такое, что
    //! этот список будет перекрывать кнопку, и из-за того, что мы используем QMenu::actionAt,
    //! то может случится ложное срабатывание, и мы запишем не "раскрытие QMenu", а "выполнение
    //! QAction".
    //! 2. Очень странная работа системы сигналов, так как при событии Press на QMenu, которое
    //! находится в QMenuBar, раскрытие происходит сразу и источник сигнала (опять же, при Press!)
    //! будет QMenu, на который мы нажали. Но при отпускании источник сигнала уже будет не QMenu,
    //! а QAction в том случае, если при раскрытии списка QMenu он перекрывает эту кнопку QMenu.
    specificWidgetMouseFilters_ = {
        filters::qMenuBarFilter,
    };

    delayedWidgetMouseFilters_ = {
        { WidgetClass::Slider, filters::qSliderFilter },
        { WidgetClass::SpinBox, filters::qSpinBoxFilter },
        { WidgetClass::Calendar, filters::qCalendarFilter },
        { WidgetClass::TreeView, filters::qTreeViewFilter },
        { WidgetClass::UndoView, filters::qUndoViewFilter },
        { WidgetClass::ItemView, filters::qItemViewSelectionFilter },
    };

    specialFilterFunctions_ = {
        filters::qCloseFilter,
    };

    keyWatchDogTimer_.setInterval(5000);
    keyWatchDogTimer_.setSingleShot(true);
    connect(&keyWatchDogTimer_, &QTimer::timeout, this, &WidgetEventFilter::callWidgetKeyFilters);
}

QString WidgetEventFilter::callWidgetMouseFilters(const QWidget *widget, const QEvent *event,
                                                  bool isContinuous, bool isSpecialEvent) noexcept
{
    // Считаем, что любое нажатие мышью или какое-либо специальное событие
    // обозначает конец редактирования текста.
    callWidgetKeyFilters();

    if (isSpecialEvent) {
        for (auto &filter : specialFilterFunctions_) {
            const auto result = filter(widget, event);
            if (!result.isEmpty()) {
                return result;
            }
        }
        return QString();
    }

    if (specificResultCanBeShown(widget)) {
        return specificResult_;
    }

    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (mouseEvent == nullptr) {
        return QString();
    }
    const auto delayedResult = callDelayedFilter(widget, mouseEvent, isContinuous);
    if (delayedResult.has_value() && !(*delayedResult).isEmpty()) {
        return *delayedResult;
    }

    for (auto &filter : widgetMouseFilters_) {
        const auto result = filter(widget, mouseEvent);
        if (!result.isEmpty()) {
            return result;
        }
    }
    return QString();
}

void WidgetEventFilter::setDelayedOrSpecificMouseEventFilter(const QWidget *widget,
                                                             const QEvent *event) noexcept
{
    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (mouseEvent == nullptr && widget == nullptr
        || (widget == delayedWidget_ && event == causedEvent_
            && causedEventType_ == QEvent::MouseButtonPress
            && event->type() == QEvent::MouseButtonDblClick)) {
        return;
    }

    destroyDelay();

    for (auto &filter : specificWidgetMouseFilters_) {
        const auto result = filter(widget, mouseEvent);
        if (!result.isEmpty()) {
            initSpecific(widget, event, std::move(result));
            return;
        }
    }

    WidgetClass foundWidgetClass = WidgetClass::None;
    std::vector<QMetaObject::Connection> connections;
    if (auto *foundWidget
        = utils::searchSpecificWidget(widget, filters::s_widgetMetaMap.at(WidgetClass::SpinBox))) {
        auto slot = [this] { this->signalDetected(); };
        foundWidgetClass = WidgetClass::SpinBox;
        QMetaObject::Connection spinBoxConnection = utils::connectIfType<QSpinBox>(
            widget, this, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), slot);
        if (!spinBoxConnection) {
            spinBoxConnection = utils::connectIfType<QDoubleSpinBox>(
                foundWidget, this,
                static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), slot);
        }
        if (!spinBoxConnection) {
            spinBoxConnection = utils::connectIfType<QDateTimeEdit>(
                foundWidget, this,
                static_cast<void (QDateTimeEdit::*)(const QDateTime &)>(
                    &QDateTimeEdit::dateTimeChanged),
                slot);
        }
        connections.push_back(spinBoxConnection);
    }
    else if (auto *foundWidget = utils::searchSpecificWidget(
                 widget, filters::s_widgetMetaMap.at(WidgetClass::Slider))) {
        foundWidgetClass = WidgetClass::Slider;
        connections.push_back(utils::connectIfType<QAbstractSlider>(
            foundWidget, this,
            static_cast<void (QAbstractSlider::*)(int)>(&QAbstractSlider::actionTriggered),
            [this](int type) {
                this->delayedExtra_.changeType = type;
                this->signalDetected();
            }));
    }
    else if (auto *foundWidget = utils::searchSpecificWidget(
                 widget, filters::s_widgetMetaMap.at(WidgetClass::Calendar))) {
        auto *itemView = qobject_cast<const QAbstractItemView *>(foundWidget);
        assert(itemView != nullptr);
        foundWidgetClass = WidgetClass::Calendar;
        connections.push_back(utils::connectIfType<QItemSelectionModel>(
            itemView->selectionModel(), this,
            static_cast<void (QItemSelectionModel::*)(const QModelIndex &, const QModelIndex &)>(
                &QItemSelectionModel::currentChanged),
            [this] { this->signalDetected(); }));
    }
    else if (auto *foundWidget = utils::searchSpecificWidget(
                 widget, filters::s_widgetMetaMap.at(WidgetClass::TreeView))) {
        foundWidgetClass = WidgetClass::TreeView;
        connections.push_back(utils::connectIfType<QTreeView>(
            foundWidget, this,
            static_cast<void (QTreeView::*)(const QModelIndex &)>(&QTreeView::expanded),
            [this](const QModelIndex &index) {
                this->delayedExtra_.changeIndex = index;
                this->delayedExtra_.changeType = ExtraInfoForDelayed::TreeViewExtra::Expanded;
                this->signalDetected();
            }));
        connections.push_back(utils::connectIfType<QTreeView>(
            foundWidget, this,
            static_cast<void (QTreeView::*)(const QModelIndex &)>(&QTreeView::collapsed),
            [this](const QModelIndex &index) {
                this->delayedExtra_.changeIndex = index;
                this->delayedExtra_.changeType = ExtraInfoForDelayed::TreeViewExtra::Collapsed;
                this->signalDetected();
            }));
    }
    else if (auto *foundWidget = utils::searchSpecificWidget(
                 widget, filters::s_widgetMetaMap.at(WidgetClass::UndoView))) {
        auto *undoView = qobject_cast<const QUndoView *>(foundWidget);
        assert(undoView != nullptr);
        foundWidgetClass = WidgetClass::UndoView;
        connections.push_back(utils::connectIfType<QUndoStack>(
            undoView->stack(), this,
            static_cast<void (QUndoStack::*)(int)>(&QUndoStack::indexChanged), [this](int index) {
                this->delayedExtra_.collectedIndexes.push_back(index);
                this->signalDetected(false);
            }));
    }
    else if (auto *foundWidget = utils::searchSpecificWidget(
                 widget, filters::s_widgetMetaMap.at(WidgetClass::ItemView))) {
        auto *itemView = qobject_cast<const QAbstractItemView *>(foundWidget);
        assert(itemView != nullptr);
        foundWidgetClass = WidgetClass::ItemView;
        connections.push_back(utils::connectIfType<QItemSelectionModel>(
            itemView->selectionModel(), this,
            static_cast<void (QItemSelectionModel::*)(const QItemSelection &,
                                                      const QItemSelection &)>(
                &QItemSelectionModel::selectionChanged),
            [this] { this->signalDetected(); }));
    }

    if (foundWidgetClass != WidgetClass::None) {
        assert(connectionIsInit(connections) == true);
        initDelay(widget, mouseEvent, delayedWidgetMouseFilters_.at(foundWidgetClass), connections);
    }
}

void WidgetEventFilter::signalDetected(bool needToDisconnect) noexcept
{
    needToUseDelayedMouseFilter_ = true;
    if (needToDisconnect) {
        disconnectAll();
    }
}

bool WidgetEventFilter::specificResultCanBeShown(const QWidget *widget) const noexcept
{
    return delayedWidget_ != nullptr
           && (widget == delayedWidget_ || widget->parent() == delayedWidget_)
           && !specificResult_.isEmpty();
}

bool WidgetEventFilter::delayedFilterCanBeCalledForWidget(const QWidget *widget) const noexcept
{
    return needToUseDelayedMouseFilter_ && !connectionIsInit() && delayedMouseFilter_.has_value()
           && delayedWidget_ != nullptr && delayedWidget_ == widget;
}

void WidgetEventFilter::initDelay(const QWidget *widget, const QMouseEvent *event,
                                  const DelayedWidgetFilterFunction &filter,
                                  std::vector<QMetaObject::Connection> &connections) noexcept
{
    causedEvent_ = event;
    causedEventType_ = event->type();
    delayedWidget_ = widget;
    delayedMouseFilter_ = filter;
    connections_ = connections;
}

void WidgetEventFilter::initSpecific(const QWidget *widget, const QEvent *event,
                                     const QString &result) noexcept
{
    causedEvent_ = event;
    causedEventType_ = event->type();
    delayedWidget_ = widget;
    specificResult_ = std::move(result);
}

void WidgetEventFilter::destroyDelay() noexcept
{
    specificResult_.clear();

    causedEventType_ = QEvent::None;
    delayedWidget_ = nullptr;
    delayedMouseFilter_ = std::nullopt;
    delayedExtra_.clear();
    needToUseDelayedMouseFilter_ = false;
    disconnectAll();
}

bool WidgetEventFilter::connectionIsInit(
    std::optional<std::vector<QMetaObject::Connection>> connections) const noexcept
{
    for (auto &connection : connections.has_value() ? *connections : connections_) {
        if (connection) {
            return true;
        }
    }
    return false;
}

void WidgetEventFilter::disconnectAll() noexcept
{
    for (auto &connection : connections_) {
        QObject::disconnect(connection);
    }
    connections_.clear();
}

std::optional<QString> WidgetEventFilter::callDelayedFilter(const QWidget *widget,
                                                            const QMouseEvent *event,
                                                            bool isContinuous) noexcept
{
    disconnectAll();
    delayedExtra_.isContinuous = isContinuous;
    bool callable = delayedFilterCanBeCalledForWidget(widget);
    return callable ? std::make_optional((*delayedMouseFilter_)(widget, event, delayedExtra_))
                    : std::nullopt;
}

void WidgetEventFilter::updateKeyWatchDog(const QWidget *widget, const QEvent *event) noexcept
{
    if (widget == nullptr || event == nullptr) {
        return;
    }

    // Изменение фокуса (при условии, что фокус переводится с уже зарегестрированного объекта)
    // считаем сигналом о завершении редактирования текста
    if (event->type() == QEvent::FocusAboutToChange && widget == keyWidget_) {
        callWidgetKeyFilters();
    }

    if (event->type() != QEvent::KeyPress) {
        return;
    }

    if (keyWidget_ != widget) {
        callWidgetKeyFilters();
    }

    //! TODO: Почему-то среди всех текстовых элементов в QtWidgets только для QKeySequenceEdit
    //! найден очень удобный сигнал editingFinished. Для остальных приходится строить систему
    //! из отслеживания фокуса и таймера. Однако может стоит придумать более надежный вариант.
    if (auto *keySeqWidget = utils::searchSpecificWidget(
            keyWidget_, filters::s_widgetMetaMap.at(WidgetClass::KeySequenceEdit))) {
        if (keySeqWidget == keyWidget_ && keyConnection_) {
            return;
        }

        keyWidget_ = keySeqWidget;
        keyConnection_ = utils::connectIfType<QKeySequenceEdit>(
            keySeqWidget, this,
            static_cast<void (QKeySequenceEdit::*)()>(&QKeySequenceEdit::editingFinished),
            [this, keySeqWidget] {
                disconnect(this->keyConnection_);
                auto *keySeqEdit = qobject_cast<const QKeySequenceEdit *>(keySeqWidget);
                assert(keySeqEdit != nullptr);
                this->flushKeyEvent(std::move(keySeqEdit->keySequence().toString()));
            });
        return;
    }

    for (const auto &widgetClass : filters::s_processedTextWidgets) {
        if (widgetClass == WidgetClass::KeySequenceEdit) {
            // KeySequenceEdit рассматриваем выше
            continue;
        }
        if (auto *foundWidget
            = utils::searchSpecificWidget(widget, filters::s_widgetMetaMap.at(widgetClass))) {
            keyWidget_ = foundWidget;
            keyWidgetClass_ = widgetClass;
            keyWatchDogTimer_.start();
            return;
        }
    }

    flushKeyEvent(filters::qKeyEventFilter(widget, event));
}

void WidgetEventFilter::callWidgetKeyFilters() noexcept
{
    if (keyWidget_ == nullptr || keyWidgetClass_ == WidgetClass::None) {
        return;
    }

    switch (keyWidgetClass_) {
    case WidgetClass::TextEdit: {
        auto *textEdit = qobject_cast<const QTextEdit *>(keyWidget_);
        assert(textEdit != nullptr);
        processKeyEvent(textEdit->toPlainText());
        return;
    }
    case WidgetClass::PlainTextEdit: {
        auto *plainTextEdit = qobject_cast<const QPlainTextEdit *>(keyWidget_);
        assert(plainTextEdit != nullptr);
        processKeyEvent(std::move(plainTextEdit->toPlainText()));
        return;
    }
    case WidgetClass::LineEdit: {
        auto *lineEdit = qobject_cast<const QLineEdit *>(keyWidget_);
        assert(lineEdit != nullptr);
        processKeyEvent(lineEdit->text());
        return;
    }
    case WidgetClass::ComboBox: {
        auto *comboBox = qobject_cast<const QComboBox *>(keyWidget_);
        assert(comboBox != nullptr);
        processKeyEvent(comboBox->currentText());
        return;
    }
    case WidgetClass::SpinBox: {
        auto *spinBox = qobject_cast<const QAbstractSpinBox *>(keyWidget_);
        assert(spinBox != nullptr);
        processKeyEvent(spinBox->text());
        return;
    }
    default:
        Q_UNREACHABLE();
    }
}

void WidgetEventFilter::processKeyEvent(const QString &text) noexcept
{
    QModelIndex index;
    const auto viewWidget = utils::searchSpecificWidget(
        keyWidget_, filters::s_widgetMetaMap.at(WidgetClass::ItemView));
    if (viewWidget != nullptr) {
        auto *view = qobject_cast<const QAbstractItemView *>(viewWidget);
        assert(view != nullptr);
        index = view->currentIndex();
    }

    const auto keyLine
        = QStringLiteral("%1('%2'%3, '%4');")
              .arg("setText")
              .arg(utils::objectPath(index.isValid() ? viewWidget : keyWidget_))
              .arg(index.isValid()
                       ? QStringLiteral(", (%1, %2)").arg(index.row()).arg(index.column())
                       : "")
              .arg(utils::escapeText(std::move(text)));
    flushKeyEvent(std::move(keyLine));

    keyWatchDogTimer_.stop();
    keyWidget_ = nullptr;
    keyWidgetClass_ = WidgetClass::None;
}

void WidgetEventFilter::flushKeyEvent(const QString &line) const noexcept
{
    assert(!line.isEmpty());
    emit newScriptKeyLine(line);
}
} // namespace QtAda::core
