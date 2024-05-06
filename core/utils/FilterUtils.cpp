#include "FilterUtils.hpp"

#include <QWidget>
#include <QModelIndex>
#include <QComboBox>
#include <QMenu>
#include <QMenuBar>
#include <QItemSelectionModel>
#include <optional>

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

QString textIndexStatement(TextIndexBehavior behavior, int index, const QString &text) noexcept
{
    if (text.isEmpty()) {
        behavior = TextIndexBehavior::OnlyIndex;
    }
    switch (behavior) {
    case TextIndexBehavior::OnlyIndex:
        return QString::number(index);
    case TextIndexBehavior::OnlyText:
        return QStringLiteral("'%1'").arg(text);
    case TextIndexBehavior::TextIndex:
        return QStringLiteral("'%1', %2").arg(text).arg(index);
    default:
        Q_UNREACHABLE();
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
        return "{row: 'ALL', column: 'ALL'}";
    }

    QStringList list;
    for (int row : fullRows) {
        list.push_back(QStringLiteral("{row: %1, column: 'ALL'}").arg(row));
    }
    for (int column : fullColumns) {
        list.push_back(QStringLiteral("{row: 'ALL', column: %1}").arg(column));
    }
    for (auto it = rowsToColumns.constBegin(); it != rowsToColumns.constEnd(); ++it) {
        if (!fullRows.contains(it.key())) {
            QStringList columns;
            for (const auto column : it.value()) {
                if (!fullColumns.contains(column)) {
                    columns.push_back(QString::number(column));
                }
            }
            if (!columns.isEmpty()) {
                list.push_back(QStringLiteral("{row: %1, column: [%2]}")
                                   .arg(it.key())
                                   .arg(columns.join(", ")));
            }
        }
    }
    return list.join(", ");
}

QString treeIndexPath(const QModelIndex &index) noexcept
{
    QStringList path;
    auto current = index;
    while (current.isValid()) {
        path.prepend(QString::number(current.row()));
        current = current.parent();
    }
    return QStringLiteral("[%1]").arg(path.join(", "));
}
} // namespace QtAda::core::utils
