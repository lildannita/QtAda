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
    { Qt::NoButton, QLatin1String("NoButton") },
    { Qt::LeftButton, QLatin1String("LeftButton") },
    { Qt::RightButton, QLatin1String("RightButton") },
    { Qt::MiddleButton, QLatin1String("MiddleButton") },
    { Qt::BackButton, QLatin1String("BackButton") },
    { Qt::ForwardButton, QLatin1String("ForwardButton") },
};

static const std::pair<ChangeType, QLatin1String> s_changeTypes[] = {
    { ChangeType::Up, QLatin1String("Up") },
    { ChangeType::DblUp, QLatin1String("DblUp") },
    { ChangeType::Down, QLatin1String("Down") },
    { ChangeType::DblDown, QLatin1String("DblDown") },
    { ChangeType::SingleStepAdd, QLatin1String("SingleStepAdd") },
    { ChangeType::SingleStepSub, QLatin1String("SingleStepSub") },
    { ChangeType::PageStepAdd, QLatin1String("PageStepAdd") },
    { ChangeType::PageStepSub, QLatin1String("PageStepSub") },
    { ChangeType::ToMinimum, QLatin1String("ToMinimum") },
    { ChangeType::ToMaximum, QLatin1String("ToMaximum") },
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
    //! TODO: В className() для классов, созданных в QML, автоматически устанавливается
    //! суффикс "QMLTYPE_{число}" (это примерно то, что мы и делаем при нахождении objectPath),
    //! но проблема в том, что это число - не постоянная величина (особенно это видно со
    //! "специфическими" надстройками над графической оболочкой). Поэтому при запуске скриптов
    //! могут быть проблемы. В связи с этим решено пока что убирать этот суффикс, если он есть.
    const QString metaObjName = QString(obj->metaObject()->className())
                                    .remove(QRegularExpression("(?<=.)_(QMLTYPE|QML)_\\d+$"));
    const auto *parent = obj->parent();
    return QString("%1_%2")
        .arg(metaObjName)
        .arg(parent ? metaObjectIndexInObjectList(obj, parent->children()) : 0);
}

static QString objectId(const QObject *obj) noexcept
{
    const auto objName = obj->objectName();
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

std::optional<Qt::MouseButton> mouseButtonFromString(const QString &mouseButton) noexcept
{
    for (const auto &pair : s_mouseButtons) {
        if (pair.second == mouseButton) {
            return pair.first;
        }
    }
    return std::nullopt;
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

QString changeTypeToString(const ChangeType type) noexcept
{
    for (const auto &pair : s_changeTypes) {
        if (pair.first == type) {
            return pair.second;
        }
    }
    return QLatin1String("<unknown>");
}

std::optional<ChangeType> changeTypeFromString(const QString &changeType) noexcept
{
    for (const auto &pair : s_changeTypes) {
        if (pair.second == changeType) {
            return pair.first;
        }
    }
    return std::nullopt;
}
} // namespace QtAda::core::utils
