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

#include "utils/CommonFilters.hpp"
#include "utils/FilterUtils.hpp"

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
    //! TODO: Компоненты QDateTimeEdit (и его потомки) имеют возможность
    //! открывать QCalendarPopup, открытие которого на текущий момент плохо обрабатывается.
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

//! TODO: нужна ли обработка зажатия кастомной кнопки?
static QString qButtonsFilter(const QWidget *widget, const QMouseEvent *event,
                              const RecordSettings &settings) noexcept
{
    Q_UNUSED(settings);
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    static const std::vector<WidgetClass> processedButtons = {
        WidgetClass::RadioButton,
        WidgetClass::CheckBox,
        // Обязательно последним:
        WidgetClass::Button,
    };

    WidgetClass currentClass = WidgetClass::None;
    const QWidget *currentWidget = nullptr;
    for (const auto &btnClass : processedButtons) {
        currentWidget = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(btnClass));
        if (currentWidget != nullptr) {
            currentClass = btnClass;
            break;
        }
    }

    if (currentClass == WidgetClass::None) {
        assert(currentWidget == nullptr);
        return QString();
    }
    assert(currentWidget != nullptr);

    bool rectContains = false;
    const auto clickPos = currentWidget->mapFromGlobal(event->globalPos());
    if (currentClass == WidgetClass::Button) {
        rectContains = currentWidget->rect().contains(clickPos);
    }
    else {
        //! TODO: Для QRadioButton и QCheckBox может быть такое, что "область нажатия"
        //! не совпадает с размерами самого объекта (для QButton такой проблемы нет),
        //! поэтому дополнительно используем minimumSizeHint(), который как раз должен
        //! и говорить о размерах области нажатия. Это, возможно, не лучший вариант
        //! проверки, поэтому в будущем нужно найти лучше.
        const auto fitSize = currentWidget->minimumSizeHint();
        const auto widgetSize = currentWidget->size();
        if (fitSize.isValid() && widgetSize.isValid()) {
            const auto clickableArea
                = QRect(QPoint(0, 0), QSize(std::min(fitSize.width(), widgetSize.width()),
                                            std::min(fitSize.height(), widgetSize.height())));
            rectContains = clickableArea.contains(clickPos);
        }
        else {
            return qMouseEventHandler(currentWidget, event);
        }
    }

    auto clickType = [rectContains, event] {
        return rectContains ? (event->type() == QEvent::MouseButtonDblClick ? "DblClick" : "Click")
                            : "Press";
    };

    auto *button = qobject_cast<const QAbstractButton *>(currentWidget);
    assert(button != nullptr);
    // Для QRadioButton, хоть он и checkable, нам это не важно, так как сколько по нему не кликай,
    // он всегда будет checked.
    const auto isCheckable
        = currentClass != WidgetClass::RadioButton ? button->isCheckable() : false;
    // Во время события Release состояние checked еще не поменяется, поэтому инвертируем значение
    const auto isChecked = !button->isChecked();
    const auto buttonText = button->text();

    if (rectContains && isCheckable) {
        const auto buttonPath = utils::objectPath(currentWidget);
        auto generate = [buttonPath, buttonText](bool isChecked) {
            return QStringLiteral("checkButton('%1', %2);%3")
                .arg(buttonPath)
                .arg(isChecked ? "true" : "false")
                .arg(buttonText.isEmpty()
                         ? ""
                         : QStringLiteral(" // Button text: '%1'").arg(buttonText.simplified()));
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
        .arg(utils::objectPath(currentWidget))
        .arg(buttonText.isEmpty()
                 ? ""
                 : QStringLiteral(" // Button text: '%1'").arg(buttonText.simplified()));
}

static QString qComboBoxFilter(const QWidget *widget, const QMouseEvent *event,
                               const RecordSettings &settings) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    size_t iteration;
    std::tie(widget, iteration) = utils::searchSpecificComponentWithIteration(
        widget, s_widgetMetaMap.at(WidgetClass::ComboBox));
    if (widget == nullptr) {
        return QString();
    }
    if (iteration <= 2) {
        return QStringLiteral("// Looks like QComboBox container clicked\n// %1")
            .arg(filters::qMouseEventHandler(widget, event));
    }

    auto *comboBox = qobject_cast<const QComboBox *>(widget);
    assert(comboBox != nullptr);

    //! TODO: нужно проверить, если выполнение кода дошло до этого места, то точно ли
    //! был нажат элемент списка
    auto *comboBoxView = comboBox->view();
    const auto containerRect = comboBoxView->rect();
    const auto clickPos = comboBoxView->mapFromGlobal(event->globalPos());

    if (containerRect.contains(clickPos)) {
        const auto index = comboBoxView->currentIndex().row();
        return QStringLiteral("selectItem('%1', %2);")
            .arg(utils::objectPath(widget))
            .arg(utils::textIndexStatement(settings.textIndexBehavior, index,
                                           comboBox->itemText(index)));
    }
    /*
     * Отпускание мыши не приведет к закрытию QListView, и если мы зарегестрируем событие
     * обычного "клика", то при воспроизведении это приведет к закрытию QListView. Поэтому
     * возвращаем комментарий, так как в противном случае, если пользователь выберет элемент
     * из списка, то это может приведет к ошибке.
     */
    return QStringLiteral(
               "// 'Release' event is outside of QComboBox, so it is still opened\n// %1")
        .arg(filters::qMouseEventHandler(widget, event));
}

static QString qSliderFilter(const QWidget *widget, const QMouseEvent *event,
                             const ExtraInfoForDelayed &extra)
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(WidgetClass::Slider));
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

    widget = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(WidgetClass::SpinBox));
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

    if (!extra.isContinuous) {
        const QRect upButtonRect(0, 0, widget->width(), widget->height() / 2);
        const QRect downButtonRect(0, widget->height() / 2, widget->width(), widget->height() / 2);
        auto generate = [&](const QLatin1String &type) {
            return utils::changeValueStatement(
                widget, QStringLiteral("%1%2")
                            .arg(event->type() == QEvent::MouseButtonDblClick ? "Dbl" : "")
                            .arg(type));
        };

        if (upButtonRect.contains(event->pos())) {
            return generate(QLatin1String("Up"));
        }
        else if (downButtonRect.contains(event->pos())) {
            return generate(QLatin1String("Down"));
        }
    }

    if (auto *spinBox = qobject_cast<const QSpinBox *>(widget)) {
        return utils::setValueStatement(widget, spinBox->value());
    }
    else if (auto *doubleSpinBox = qobject_cast<const QDoubleSpinBox *>(widget)) {
        return utils::setValueStatement(widget, doubleSpinBox->value());
    }
    else {
        Q_UNREACHABLE();
    }
}

static QString qCalendarFilter(const QWidget *widget, const QMouseEvent *event,
                               const ExtraInfoForDelayed &extra) noexcept
{
    Q_UNUSED(extra);
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(WidgetClass::Calendar));
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
        .arg(utils::setValueStatement(qobject_cast<const QWidget *>(calendar),
                                      currentDate.toString(Qt::ISODate)));
}

static QString qTreeViewFilter(const QWidget *widget, const QMouseEvent *event,
                               const ExtraInfoForDelayed &extra) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(WidgetClass::TreeView));
    if (widget == nullptr) {
        return QString();
    }

    // В этом фильтре обрабатываем только Expanded и Collapsed события для QTreeView,
    // для остальных событий будет вызван фильтр qItemViewFilter
    assert(extra.changeType.has_value());
    assert(*extra.changeType == ExtraInfoForDelayed::TreeViewExtra::Collapsed
           || *extra.changeType == ExtraInfoForDelayed::TreeViewExtra::Expanded);
    assert(extra.changeModelIndex.isValid());

    auto *view = qobject_cast<const QAbstractItemView *>(widget);
    assert(view != nullptr);

    const auto currentItem = view->model()->data(extra.changeModelIndex);
    const auto currentItemText
        = currentItem.canConvert<QString>() ? currentItem.toString() : QString();
    return QStringLiteral("%1Delegate('%2', %3);%4")
        .arg(extra.changeType == ExtraInfoForDelayed::TreeViewExtra::Expanded ? "expand"
                                                                              : "collapse")
        .arg(utils::objectPath(widget))
        .arg(utils::treeIndexPath(extra.changeModelIndex))
        .arg(currentItemText.isEmpty()
                 ? ""
                 : QStringLiteral(" // Delegate text: '%1'").arg(currentItemText.simplified()));
}

static QString qUndoViewFilter(const QWidget *widget, const QMouseEvent *event,
                               const ExtraInfoForDelayed &extra) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event) || extra.collectedIndexes.empty()) {
        return QString();
    }

    widget = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(WidgetClass::ItemView));
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
                      .arg(utils::objectPath(widget))
                      .arg(index)
                      .arg(currentItemText.isEmpty() ? ""
                                                     : QStringLiteral(" // Delegate text: '%1'")
                                                           .arg(currentItemText.simplified()));
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
        const auto indexPath
            = qobject_cast<const QTreeView *>(view) != nullptr
                  ? utils::treeIndexPath(currentIndex)
                  : QStringLiteral("%1, %2").arg(currentIndex.row()).arg(currentIndex.column());
        return QStringLiteral("delegate%1Click('%2', %3);%4")
            .arg(event->type() == QEvent::MouseButtonDblClick ? "Dbl" : "")
            .arg(utils::objectPath(qobject_cast<const QWidget *>(view)))
            .arg(indexPath)
            .arg(currentItemText.isEmpty()
                     ? ""
                     : QStringLiteral(" // Delegate text: '%1'").arg(currentItemText.simplified()));
    }

    return QString();
}

static QString qItemViewFilter(const QWidget *widget, const QMouseEvent *event,
                               const RecordSettings &settings) noexcept
{
    Q_UNUSED(settings);
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    if (utils::searchSpecificComponent(widget, s_widgetMetaMap.at(WidgetClass::ColumnViewGrip))
        != nullptr) {
        // Никак дополнительно не обрабатываем это действие, так как оно не влияет
        // на функционал, а влияет только на визуальное отображение элементов
        return QLatin1String("// Looks like QColumnViewGrip moved");
    }

    widget = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(WidgetClass::ItemView));
    if (widget == nullptr) {
        return QString();
    }

    /*
     * События для QUndoView обрабатываем отдельно, но если индекс в QUndoView, то никакого
     * полезного события и не будет, следовательно, обработчик дойдет до сюда, поэтому необходимо
     * отметить, что данный клик бесполезен.
     */
    bool isUndoView
        = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(WidgetClass::UndoView))
          != nullptr;
    /*
     * События для QCalendarView обрабатываем отдельно, но если мы нажали на уже выбранную дату,
     * или нажали на "пустое" место, то то никакого полезного события и не будет, следовательно,
     * обработчик дойдет до сюда, поэтому необходимо отметить, что данный клик бесполезен.
     */
    //! TODO: для QCalendarView проблема в том, что если мы нажали на "пустое" место или на
    //! "номер" месяца, то текущая реализация все равно сгенерирует сообщение о клике на уже
    //! выбранный делегат.
    bool isCalendarView
        = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(WidgetClass::Calendar))
          != nullptr;

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
            return QStringLiteral("clearSelection('%1');").arg(utils::objectPath(widget));
        }
        return QString();
    }

    return QStringLiteral("%1%2")
        .arg(isUndoView ? "// Looks like QUndoView useless delegate click\n// "
                        : (isCalendarView ? "// Looks like QCalendarView useless click\n// " : ""))
        .arg(clickResult);
}

static QString qItemViewSelectionFilter(const QWidget *widget, const QMouseEvent *event,
                                        const ExtraInfoForDelayed &extra) noexcept
{
    Q_UNUSED(extra);
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(WidgetClass::ItemView));
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
    return selectedCellsData.isEmpty()
               ? QStringLiteral("clearSelection('%1');").arg(utils::objectPath(widget))
               : QStringLiteral("let selectionData = [%1];\nsetSelection('%2', selectionData);")
                     .arg(selectedCellsData)
                     .arg(utils::objectPath(widget));
}

static QString qMenuBarFilter(const QWidget *widget, const QMouseEvent *event,
                              const RecordSettings &settings) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event, true)) {
        return QString();
    }

    widget = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(WidgetClass::MenuBar));
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

    const auto *menu = action->menu();
    if (menu == nullptr) {
        return QStringLiteral("%1activateMenuAction('%2', %3%4);")
            .arg(action->isSeparator() ? "// Looks like QMenu::Separator clicked\n// " : "")
            .arg(utils::objectPath(widget))
            .arg(utils::textIndexStatement(settings.textIndexBehavior,
                                           menuBar->actions().indexOf(action), action->text()))
            .arg(action->isCheckable()
                     ? QStringLiteral(", %1").arg(action->isChecked() ? "false" : "true")
                     : "");
    }
    else {
        const auto menuText = menu->title();
        auto *menuWidget = qobject_cast<const QWidget *>(menu);
        return QStringLiteral("activateMenu('%1');%2")
            .arg(utils::objectPath(menuWidget))
            .arg(menuText.isEmpty()
                     ? ""
                     : QStringLiteral(" // Menu title: '%1'").arg(menuText.simplified()));
    }
}

static QString qMenuFilter(const QWidget *widget, const QMouseEvent *event,
                           const RecordSettings &settings) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(WidgetClass::Menu));
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
            .arg(menuText.isEmpty()
                     ? ""
                     : QStringLiteral(" // Menu title: '%1'").arg(menuText.simplified()));
    }
    else {
        return QStringLiteral("%1activateMenuAction('%2', %3%4);")
            .arg(action->isSeparator() ? " // Looks like QMenu::Separator clicked\n// " : "")
            .arg(utils::objectPath(widget))
            .arg(utils::textIndexStatement(settings.textIndexBehavior,
                                           menu->actions().indexOf(action), action->text()))
            .arg(action->isCheckable()
                     ? QStringLiteral(", %1").arg(action->isChecked() ? "false" : "true")
                     : "");
    }
}

static QString qTabBarFilter(const QWidget *widget, const QMouseEvent *event,
                             const RecordSettings &settings) noexcept
{
    if (!utils::mouseEventCanBeFiltered(widget, event)) {
        return QString();
    }

    widget = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(WidgetClass::TabBar));
    if (widget == nullptr) {
        return QString();
    }

    auto *tabBar = qobject_cast<const QTabBar *>(widget);
    assert(tabBar != nullptr);
    const auto index = tabBar->currentIndex();
    return QStringLiteral("selectTabItem('%1', %2);")
        .arg(utils::objectPath(widget))
        .arg(utils::textIndexStatement(settings.textIndexBehavior, index, tabBar->tabText(index)));
}

static QString qTextFocusFilters(const QWidget *widget, const QMouseEvent *event,
                                 const RecordSettings &settings) noexcept
{
    Q_UNUSED(settings);
    for (const auto &widgetClass : s_processedTextWidgets) {
        if (auto *foundWidget
            = utils::searchSpecificComponent(widget, s_widgetMetaMap.at(widgetClass))) {
            QLatin1String widgetClassStr;
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

            return QStringLiteral("// Looks like focus click on %1\n// %2")
                .arg(widgetClassStr)
                .arg(filters::qMouseEventHandler(foundWidget, event));
        }
    }
    return QString();
}
} // namespace QtAda::core::filters

namespace QtAda::core {
WidgetEventFilter::WidgetEventFilter(const RecordSettings &settings, QObject *parent) noexcept
    : GuiEventFilter{ settings, parent }
{
    mouseFilters_ = {
        filters::qComboBoxFilter,
        filters::qMenuFilter,
        filters::qTabBarFilter,
        filters::qItemViewFilter,
        // Обязательно в таком порядке:
        filters::qButtonsFilter,
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
    specificMouseFilters_ = {
        filters::qMenuBarFilter,
    };

    signalMouseFilters_ = {
        { WidgetClass::Slider, filters::qSliderFilter },
        { WidgetClass::SpinBox, filters::qSpinBoxFilter },
        { WidgetClass::Calendar, filters::qCalendarFilter },
        { WidgetClass::TreeView, filters::qTreeViewFilter },
        { WidgetClass::UndoView, filters::qUndoViewFilter },
        { WidgetClass::ItemView, filters::qItemViewSelectionFilter },
    };
}

std::pair<QString, bool> WidgetEventFilter::callMouseFilters(const QObject *obj,
                                                             const QEvent *event,
                                                             bool isContinuous) noexcept
{
    const std::pair<QString, bool> empty = { QString(), false };
    auto *widget = qobject_cast<const QWidget *>(obj);
    if (widget == nullptr) {
        return empty;
    }

    // Считаем, что любое нажатие мышью или какое-либо специальное событие
    // обозначает конец редактирования текста.
    callKeyFilters();

    if (delayedWatchDog_.specificResultCanBeShown(widget)) {
        return { delayedWatchDog_.specificResult, false };
    }

    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (mouseEvent == nullptr) {
        return empty;
    }
    const auto delayedResult = delayedWatchDog_.callDelayedFilter(widget, mouseEvent, isContinuous);
    if (delayedResult.has_value() && !(*delayedResult).isEmpty()) {
        return { *delayedResult, false };
    }

    for (auto &filter : mouseFilters_) {
        const auto result = filter(widget, mouseEvent, recordSettings_);
        if (!result.isEmpty()) {
            return { result, false };
        }
    }
    return empty;
}

void WidgetEventFilter::setMousePressFilter(const QObject *obj, const QEvent *event) noexcept
{
    auto *widget = qobject_cast<const QWidget *>(obj);
    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (mouseEvent == nullptr || widget == nullptr
        || (widget == delayedWatchDog_.causedComponent && event == delayedWatchDog_.causedEvent
            && delayedWatchDog_.causedEventType == QEvent::MouseButtonPress
            && event->type() == QEvent::MouseButtonDblClick)) {
        return;
    }

    delayedWatchDog_.clear();

    for (auto &filter : specificMouseFilters_) {
        const auto result = filter(widget, mouseEvent, recordSettings_);
        if (!result.isEmpty()) {
            delayedWatchDog_.initSpecific(widget, event, std::move(result));
            return;
        }
    }

    WidgetClass foundWidgetClass = WidgetClass::None;
    std::vector<QMetaObject::Connection> connections;
    if (auto *foundWidget = utils::searchSpecificComponent(
            widget, filters::s_widgetMetaMap.at(WidgetClass::SpinBox))) {
        auto slot = [this] { this->delayedWatchDog_.processSignal(); };
        foundWidgetClass = WidgetClass::SpinBox;
        QMetaObject::Connection spinBoxConnection = utils::connectIfType<QSpinBox>(
            widget, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, slot);
        if (!spinBoxConnection) {
            spinBoxConnection = utils::connectIfType<QDoubleSpinBox>(
                foundWidget,
                static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
                slot);
        }
        if (!spinBoxConnection) {
            spinBoxConnection = utils::connectIfType<QDateTimeEdit>(
                foundWidget,
                static_cast<void (QDateTimeEdit::*)(const QDateTime &)>(
                    &QDateTimeEdit::dateTimeChanged),
                this, slot);
        }
        connections.push_back(spinBoxConnection);
    }
    else if (auto *foundWidget = utils::searchSpecificComponent(
                 widget, filters::s_widgetMetaMap.at(WidgetClass::Slider))) {
        foundWidgetClass = WidgetClass::Slider;
        connections.push_back(utils::connectIfType<QAbstractSlider>(
            foundWidget,
            static_cast<void (QAbstractSlider::*)(int)>(&QAbstractSlider::actionTriggered), this,
            [this](int type) {
                this->delayedWatchDog_.extra.changeType = type;
                this->delayedWatchDog_.processSignal();
            }));
    }
    else if (auto *foundWidget = utils::searchSpecificComponent(
                 widget, filters::s_widgetMetaMap.at(WidgetClass::Calendar))) {
        auto *itemView = qobject_cast<const QAbstractItemView *>(foundWidget);
        assert(itemView != nullptr);
        foundWidgetClass = WidgetClass::Calendar;
        connections.push_back(utils::connectObject(
            itemView->selectionModel(),
            static_cast<void (QItemSelectionModel::*)(const QModelIndex &, const QModelIndex &)>(
                &QItemSelectionModel::currentChanged),
            this, [this] { this->delayedWatchDog_.processSignal(); }));
    }
    else if (auto *foundWidget = utils::searchSpecificComponent(
                 widget, filters::s_widgetMetaMap.at(WidgetClass::TreeView))) {
        foundWidgetClass = WidgetClass::TreeView;
        connections.push_back(utils::connectIfType<QTreeView>(
            foundWidget,
            static_cast<void (QTreeView::*)(const QModelIndex &)>(&QTreeView::expanded), this,
            [this](const QModelIndex &index) {
                this->delayedWatchDog_.extra.changeModelIndex = index;
                this->delayedWatchDog_.extra.changeType
                    = ExtraInfoForDelayed::TreeViewExtra::Expanded;
                this->delayedWatchDog_.processSignal();
            }));
        connections.push_back(utils::connectIfType<QTreeView>(
            foundWidget,
            static_cast<void (QTreeView::*)(const QModelIndex &)>(&QTreeView::collapsed), this,
            [this](const QModelIndex &index) {
                this->delayedWatchDog_.extra.changeModelIndex = index;
                this->delayedWatchDog_.extra.changeType
                    = ExtraInfoForDelayed::TreeViewExtra::Collapsed;
                this->delayedWatchDog_.processSignal();
            }));
    }
    else if (auto *foundWidget = utils::searchSpecificComponent(
                 widget, filters::s_widgetMetaMap.at(WidgetClass::UndoView))) {
        auto *undoView = qobject_cast<const QUndoView *>(foundWidget);
        assert(undoView != nullptr);
        foundWidgetClass = WidgetClass::UndoView;
        connections.push_back(utils::connectObject(
            undoView->stack(), static_cast<void (QUndoStack::*)(int)>(&QUndoStack::indexChanged),
            this, [this](int index) {
                this->delayedWatchDog_.extra.collectedIndexes.push_back(index);
                this->delayedWatchDog_.processSignal(false);
            }));
    }
    else if (auto *foundWidget = utils::searchSpecificComponent(
                 widget, filters::s_widgetMetaMap.at(WidgetClass::ItemView))) {
        auto *itemView = qobject_cast<const QAbstractItemView *>(foundWidget);
        assert(itemView != nullptr);
        foundWidgetClass = WidgetClass::ItemView;
        connections.push_back(
            QObject::connect(itemView->selectionModel(),
                             static_cast<void (QItemSelectionModel::*)(const QItemSelection &,
                                                                       const QItemSelection &)>(
                                 &QItemSelectionModel::selectionChanged),
                             this, [this] { this->delayedWatchDog_.processSignal(); }));
    }

    if (foundWidgetClass != WidgetClass::None) {
        assert(utils::connectionIsInit(connections) == true);
        delayedWatchDog_.initDelay(widget, mouseEvent, signalMouseFilters_.at(foundWidgetClass),
                                   connections);
    }
}

void WidgetEventFilter::handleKeyEvent(const QObject *obj, const QEvent *event) noexcept
{
    auto *widget = qobject_cast<const QWidget *>(obj);
    if (widget == nullptr || event == nullptr) {
        return;
    }

    // Изменение фокуса (при условии, что фокус переводится с уже зарегестрированного объекта)
    // считаем сигналом о завершении редактирования текста
    if (event->type() == QEvent::FocusAboutToChange && widget == keyWatchDog_.component) {
        callKeyFilters();
    }

    if (event->type() != QEvent::KeyPress) {
        return;
    }

    if (keyWatchDog_.component != widget) {
        callKeyFilters();
    }

    //! TODO: Почему-то среди всех текстовых элементов в QtWidgets только для QKeySequenceEdit
    //! найден очень удобный сигнал editingFinished. Для остальных приходится строить систему
    //! из отслеживания фокуса и таймера. Однако может стоит придумать более надежный вариант.
    if (auto *keySeqWidget = utils::searchSpecificComponent(
            keyWatchDog_.component, filters::s_widgetMetaMap.at(WidgetClass::KeySequenceEdit))) {
        if (keySeqWidget == keyWatchDog_.component && keyWatchDog_.connection) {
            return;
        }

        keyWatchDog_.component = keySeqWidget;
        keyWatchDog_.connection = utils::connectIfType<QKeySequenceEdit>(
            keySeqWidget,
            static_cast<void (QKeySequenceEdit::*)()>(&QKeySequenceEdit::editingFinished), this,
            [this, keySeqWidget] {
                QObject::disconnect(this->keyWatchDog_.connection);
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
            = utils::searchSpecificComponent(widget, filters::s_widgetMetaMap.at(widgetClass))) {
            keyWatchDog_.component = foundWidget;
            keyWatchDog_.componentClass = widgetClass;
            keyWatchDog_.timer.start();
            return;
        }
    }

    flushKeyEvent(filters::qKeyEventHandler(widget, event));
}

void WidgetEventFilter::callKeyFilters() noexcept
{
    if (keyWatchDog_.component == nullptr || keyWatchDog_.componentClass == WidgetClass::None) {
        return;
    }

    auto *keyWidget = keyWatchDog_.component;
    switch (keyWatchDog_.componentClass) {
    case WidgetClass::TextEdit: {
        auto *textEdit = qobject_cast<const QTextEdit *>(keyWidget);
        assert(textEdit != nullptr);
        processKeyEvent(textEdit->toPlainText());
        return;
    }
    case WidgetClass::PlainTextEdit: {
        auto *plainTextEdit = qobject_cast<const QPlainTextEdit *>(keyWidget);
        assert(plainTextEdit != nullptr);
        processKeyEvent(plainTextEdit->toPlainText());
        return;
    }
    case WidgetClass::LineEdit: {
        auto *lineEdit = qobject_cast<const QLineEdit *>(keyWidget);
        assert(lineEdit != nullptr);
        processKeyEvent(lineEdit->text());
        return;
    }
    case WidgetClass::ComboBox: {
        auto *comboBox = qobject_cast<const QComboBox *>(keyWidget);
        assert(comboBox != nullptr);
        processKeyEvent(comboBox->currentText());
        return;
    }
    case WidgetClass::SpinBox: {
        auto *spinBox = qobject_cast<const QAbstractSpinBox *>(keyWidget);
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
    assert(keyWatchDog_.component != nullptr);

    QModelIndex index;
    QString indexPath;
    const auto viewWidget = utils::searchSpecificComponent(
        keyWatchDog_.component, filters::s_widgetMetaMap.at(WidgetClass::ItemView));
    if (viewWidget != nullptr) {
        auto *view = qobject_cast<const QAbstractItemView *>(viewWidget);
        assert(view != nullptr);
        index = view->currentIndex();
        if (index.isValid()) {
            if (qobject_cast<const QTreeView *>(view) != nullptr) {
                indexPath = utils::treeIndexPath(index);
            }
            else {
                indexPath = QStringLiteral("%1, %2").arg(index.row()).arg(index.column());
            }
        }
    }

    const auto keyLine
        = QStringLiteral("setText('%1'%2, '%3');")
              .arg(utils::objectPath(index.isValid() ? viewWidget : keyWatchDog_.component))
              .arg(indexPath.isEmpty() ? "" : QStringLiteral(", %1").arg(indexPath))
              .arg(utils::escapeText(std::move(text)));
    flushKeyEvent(std::move(keyLine));

    keyWatchDog_.clear();
}

QString WidgetEventFilter::handleCloseEvent(const QObject *obj, const QEvent *event) noexcept
{
    if (obj == nullptr || event == nullptr
        || (event != nullptr && event->type() != QEvent::Close)) {
        return QString();
    }

    auto *widget = qobject_cast<const QWidget *>(obj);
    assert(widget != nullptr);

    if (utils::searchSpecificComponent(widget, filters::s_widgetMetaMap.at(WidgetClass::Dialog))
        != nullptr) {
        return QStringLiteral("closeDialog(%1);").arg(utils::objectPath(widget));
    }
    else if (utils::searchSpecificComponent(widget,
                                            filters::s_widgetMetaMap.at(WidgetClass::Window))
             != nullptr) {
        return QStringLiteral("closeWindow(%1);").arg(utils::objectPath(widget));
    }
    //! TODO: Это событие для QMenu генерируется и при выборе какого-либо QAction, причем
    //! событие мыши, которое привело к закрытию QMenu, не генерируется (в отличие от
    //! QQuickMenu). Позже нужно будет решить, что делать с QMenu - нужно ли вообще нам
    //! генерировать их открытие/закрытие, так как нам по идее важны только выполнение QAction,
    //! а раскрытие/закрытие QMenu ни к чему не приводит.
    //!
    //! UPD: Как оказалось, такое генерируется и не только для QMenu, но и для QToolBar.
    //! Поэтому пока что делаем заглушку для всех остальных случаев:
    return QStringLiteral("// close(%1);").arg(utils::objectPath(widget));
}
} // namespace QtAda::core
