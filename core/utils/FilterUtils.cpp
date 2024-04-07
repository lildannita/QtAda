#include "FilterUtils.hpp"

#include <QMouseEvent>
#include <QModelIndex>

#include <QComboBox>
#include <QMenu>
#include <QMenuBar>
#include <QItemSelectionModel>

//! TODO: remove
#include <iostream>

namespace QtAda::core::utils {
static const std::pair<Qt::MouseButton, QLatin1String> s_mouseButtons[] = {
    { Qt::NoButton, QLatin1String("Qt::NoButton") },
    { Qt::LeftButton, QLatin1String("Qt::LeftButton") },
    { Qt::RightButton, QLatin1String("Qt::RightButton") },
    { Qt::MiddleButton, QLatin1String("Qt::MiddleButton") },
    { Qt::BackButton, QLatin1String("Qt::BackButton") },
    { Qt::ForwardButton, QLatin1String("Qt::ForwardButton") },
};

static const std::vector<std::pair<char, QString>> s_escapeReplacements
    = { { '\n', "\\n" }, { '\r', "\\r" }, { '\t', "\\t" }, { '\v', "\\v" } };

static uint metaObjectIndexInObjectList(const QObject *obj, const QObjectList &children) noexcept
{
    assert(!children.isEmpty());
    uint index = 0;
    const auto className = obj->metaObject()->className();
    for (const QObject *item : children) {
        if (item == obj) {
            return index;
        }
        if (className == item->metaObject()->className()) {
            index++;
        }
    }
    Q_UNREACHABLE();
}

static uint objectIndexInObjectList(const QObject *obj, const QObjectList &children) noexcept
{
    assert(!children.isEmpty());
    uint index = 0;
    const auto objName = obj->objectName();
    for (const QObject *item : children) {
        if (item == obj) {
            return index;
        }
        if (objName == item->objectName()) {
            index++;
        }
    }
    Q_UNREACHABLE();
}

static QString metaObjectId(const QObject *obj) noexcept
{
    const QString metaObjName = obj->metaObject()->className();
    const auto *parent = obj->parent();
    return QString("%1_%2")
        .arg(metaObjName)
        .arg(parent ? metaObjectIndexInObjectList(obj, parent->children()) : 0);
}

static QString objectId(const QObject *obj) noexcept
{
    const QString objName = obj->objectName();
    const auto *parent = obj->parent();
    return QString("%1_%2").arg(objName).arg(
        parent ? objectIndexInObjectList(obj, parent->children()) : 0);
}

QString objectPath(const QObject *obj) noexcept
{
    QStringList pathComponents;
    while (obj != nullptr) {
        QString identifier = obj->objectName().isEmpty() ? QString("c=%1").arg(metaObjectId(obj))
                                                         : QString("n=%1").arg(objectId(obj));
        pathComponents.prepend(identifier);
        obj = obj->parent();
    }
    assert(!pathComponents.isEmpty());
    return pathComponents.join('/');
}

QString escapeText(const QString &text) noexcept
{
    QString result = text;
    for (const auto &replacement : s_escapeReplacements) {
        result.replace(replacement.first, replacement.second, Qt::CaseSensitive);
    }
    return result;
}

QString mouseButtonToString(const Qt::MouseButton mouseButton) noexcept
{
    for (const auto &pair : s_mouseButtons) {
        if (pair.first == mouseButton) {
            return pair.second;
        }
    }
    return QLatin1String("<unknown>");
}

bool mouseEventCanBeFiltered(const QWidget *widget, const QMouseEvent *event,
                             bool shouldBePressEvent) noexcept
{
    const auto type = event->type();
    return widget != nullptr && event != nullptr && event->button() == Qt::LeftButton
           && (type == (shouldBePressEvent ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease)
               || type == QEvent::MouseButtonDblClick);
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
    case ComboBox: {
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
    case Menu: {
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
    case MenuBar: {
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
    case TabBar: {
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
