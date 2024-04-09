#include "WidgetFilterUtils.hpp"

#include <QObject>
#include <QWidget>
#include <QMouseEvent>
#include <QModelIndex>
#include <QComboBox>
#include <QMenu>
#include <QMenuBar>
#include <QItemSelectionModel>

#include "CommonFilterUtils.hpp"

namespace QtAda::core::utils {
QString objectPath(const QWidget *widget) noexcept
{
    auto *obj = qobject_cast<const QObject *>(widget);
    return objectPath(obj);
}

bool mouseEventCanBeFiltered(const QWidget *widget, const QMouseEvent *event,
                             bool shouldBePressEvent) noexcept
{
    return widget != nullptr && mouseEventCanBeFiltered(event, shouldBePressEvent);
}

std::pair<const QWidget *, size_t>
searchSpecificWidgetWithIteration(const QWidget *widget,
                                  const std::pair<QLatin1String, size_t> &classDesignation) noexcept
{
    for (size_t i = 1; i <= classDesignation.second && widget != nullptr; i++) {
        const auto *metaObject = widget->metaObject();
        while (metaObject != nullptr) {
            if (classDesignation.first == metaObject->className()) {
                return std::make_pair(widget, i);
            }
            metaObject = metaObject->superClass();
        }
        widget = widget->parentWidget();
    }
    return std::make_pair(nullptr, 0);
}

const QWidget *
searchSpecificWidget(const QWidget *widget,
                     const std::pair<QLatin1String, size_t> &classDesignation) noexcept
{
    return searchSpecificWidgetWithIteration(widget, classDesignation).first;
}

QString widgetIdInView(const QWidget *widget, const int index,
                       const WidgetClass widgetClass) noexcept
{
    switch (widgetClass) {
    case WidgetClass::ComboBox: {
        auto *comboBox = qobject_cast<const QComboBox *>(widget);
        assert(comboBox != nullptr);
        const auto itemText = comboBox->itemText(index);
        for (int i = 0, itemIndex = 0; i < comboBox->count(); i++) {
            if (i == index) {
                return QStringLiteral("%1_%2").arg(itemText).arg(itemIndex);
            }
            if (itemText == comboBox->itemText(i)) {
                itemIndex++;
            }
        }
        Q_UNREACHABLE();
    }
    case WidgetClass::Menu: {
        auto *menu = qobject_cast<const QMenu *>(widget);
        assert(menu != nullptr);
        auto *action = menu->actions().at(index);
        assert(action != nullptr);
        const auto actionText = action->text();
        for (int i = 0, actionIndex = 0; i < menu->actions().count(); i++) {
            if (i == index) {
                return QStringLiteral("%1_%2").arg(actionText).arg(actionIndex);
            }
            if (actionText == menu->actions().at(i)->text()) {
                actionIndex++;
            }
        }
        Q_UNREACHABLE();
    }
    case WidgetClass::MenuBar: {
        auto *menuBar = qobject_cast<const QMenuBar *>(widget);
        assert(menuBar != nullptr);
        auto *action = menuBar->actions().at(index);
        assert(action != nullptr);
        const auto actionText = action->text();
        for (int i = 0, actionIndex = 0; i < menuBar->actions().count(); i++) {
            if (i == index) {
                return QStringLiteral("%1_%2").arg(actionText).arg(actionIndex);
            }
            if (actionText == menuBar->actions().at(i)->text()) {
                actionIndex++;
            }
        }
        Q_UNREACHABLE();
    }
    case WidgetClass::TabBar: {
        auto *tabBar = qobject_cast<const QTabBar *>(widget);
        assert(tabBar != nullptr);
        const auto itemText = tabBar->tabText(index);
        for (int i = 0, itemIndex = 0; i < tabBar->count(); i++) {
            if (i == index) {
                return QStringLiteral("%1_%2").arg(itemText).arg(itemIndex);
            }
            if (itemText == tabBar->tabText(i)) {
                itemIndex++;
            }
        }
        Q_UNREACHABLE();
    }
    default:
        return QString();
    }
}

QString selectedCellsData(const QItemSelectionModel *selectionModel) noexcept
{
    auto model = selectionModel->model();
    QMap<int, QSet<int>> rowsToColumns;
    QSet<int> fullRows, fullColumns;

    for (const QModelIndex &index : selectionModel->selectedIndexes()) {
        rowsToColumns[index.row()].insert(index.column());
    }
    for (auto it = rowsToColumns.constBegin(); it != rowsToColumns.constEnd(); ++it) {
        if (it.value().size() == model->columnCount()) {
            fullRows.insert(it.key());
        }
    }
    for (int column = 0; column < model->columnCount(); ++column) {
        bool fullColumn = true;
        for (int row = 0; row < model->rowCount(); ++row) {
            if (!rowsToColumns.contains(row) || !rowsToColumns[row].contains(column)) {
                fullColumn = false;
                break;
            }
        }
        if (fullColumn) {
            fullColumns.insert(column);
        }
    }

    if (fullRows.size() == model->rowCount() && fullColumns.size() == model->columnCount()) {
        return "['ALL', 'ALL']";
    }

    QString result;
    for (int row : fullRows) {
        result += QStringLiteral("[%1, 'ALL'], ").arg(row);
    }
    for (int column : fullColumns) {
        result += QStringLiteral("['ALL', %1], ").arg(column);
    }
    for (auto it = rowsToColumns.constBegin(); it != rowsToColumns.constEnd(); ++it) {
        if (!fullRows.contains(it.key())) {
            QString cols = "[";
            for (int col : it.value()) {
                if (!fullColumns.contains(col)) {
                    cols += QStringLiteral("%1, ").arg(col);
                }
            }
            if (cols != "[") {
                cols[cols.length() - 2] = ']';
                cols.chop(1);
                result += QStringLiteral("[%1, %2], ").arg(it.key()).arg(cols);
            }
        }
    }

    if (!result.isEmpty()) {
        result.chop(2);
    }

    return result;
}
} // namespace QtAda::core::utils
