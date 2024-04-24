#include "PropertiesWatcher.hpp"

#include <QStandardItem>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTreeView>
#include <QTableView>
#include <QHeaderView>
#include <vector>
#include <QSplitter>
#include <QLabel>

#include "InprocessTools.hpp"

namespace QtAda::inprocess {
static void cloneModelItems(const QStandardItem *sourceItem, QStandardItem *targetItem) {
    if (!sourceItem || !targetItem) {
        return;
    }

    for (int row = 0; row < sourceItem->rowCount(); ++row) {
        for (int column = 0; column < sourceItem->columnCount(); ++column) {
            QStandardItem *child = sourceItem->child(row, column);
            if (child) {
                QStandardItem *newChild = child->clone();
                targetItem->setChild(row, column, newChild);
                cloneModelItems(child, newChild);
            }
        }
    }
}

static std::deque<int> getItemPath(const QModelIndex &index) {
    std::deque<int> path;
    QModelIndex current = index;
    while (current.isValid()) {
        path.push_front(current.row());
        current = current.parent();
    }
    return path;
}

PropertiesWatcher::PropertiesWatcher(InprocessController *inprocessController, QWidget *parent) noexcept
    : QWidget{ parent }
    , inprocessController_{ inprocessController }
    , acceptSelectionButton_{ new QPushButton }
    , treeView_{ new QTreeView }
    , framedObjectModel_{ new QStandardItemModel(this) }
    , tableView_{ new QTableView }
    , metaPropertyModel_{ new QStandardItemModel(this) }
    , contentWidget_{ new QWidget }
    , placeholderLabel_{ new QLabel }
{
    connect(inprocessController_, &InprocessController::newFramedRootObjectData, this, &PropertiesWatcher::setFramedRootObjectData);
    connect(inprocessController_, &InprocessController::newMetaPropertyData, this, &PropertiesWatcher::setMetaPropertyModel);

    // Инициализация QTreeView, отображающего дерево элементов
    treeView_->setHeaderHidden(true);
    treeView_->setModel(framedObjectModel_);

    // Инициализация QTableView, отображающего свойства выбранного элемента
    tableView_->setModel(metaPropertyModel_);
    tableView_->verticalHeader()->setVisible(false);
    tableView_->horizontalHeader()->setSectionsClickable(false);
    tableView_->horizontalHeader()->setHighlightSections(false);
    tableView_->setSelectionMode(QAbstractItemView::MultiSelection);
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(tableView_->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &PropertiesWatcher::metaSelectionChanged);

    // Инициализация растягивающегося макета под View-компоненты
    QSplitter *viewsSplitter = new QSplitter(Qt::Vertical);
    viewsSplitter->setChildrenCollapsible(false);
    viewsSplitter->addWidget(treeView_);
    viewsSplitter->addWidget(tableView_);

    // Инициализация кнопок, управляющих выбором свойств инспектируемого объекта
    QPushButton *selectAllButton = new QPushButton;
    QPushButton *clearSelectionButton = new QPushButton;
    initButton(selectAllButton, "Select All");
    initButton(clearSelectionButton, "Clear Selection");
    initButton(acceptSelectionButton_, "Accept Selection");
    connect(selectAllButton, &QPushButton::clicked, tableView_, &QAbstractItemView::selectAll);
    connect(clearSelectionButton, &QPushButton::clicked, tableView_,
            &QAbstractItemView::clearSelection);
    connect(acceptSelectionButton_, &QPushButton::clicked, this,
            &PropertiesWatcher::acceptSelection);
    acceptSelectionButton_->setEnabled(false);

    // Инициализация макета с кнопками
    QWidget *buttonsWidget = new QWidget;
    QHBoxLayout *buttonsLayout = new QHBoxLayout(buttonsWidget);
    buttonsLayout->addWidget(selectAllButton);
    buttonsLayout->addWidget(clearSelectionButton);
    buttonsLayout->addWidget(generateSeparator(this));
    buttonsLayout->addWidget(acceptSelectionButton_);

    // Инициализация макета под основные компоненты
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget_);
    contentLayout->addWidget(viewsSplitter);
    contentLayout->addWidget(buttonsWidget);
    contentWidget_->setVisible(false);

    // Инициализация плейсхолдера
    placeholderLabel_ = new QLabel(
        "Currently, there is nothing to display as no GUI component is selected."
        "To view the properties list, please click on the desired component in the GUI.");
    placeholderLabel_->setWordWrap(true);
    placeholderLabel_->setAlignment(Qt::AlignCenter);
    placeholderLabel_->setFixedHeight(contentWidget_->sizeHint().height());

    // Инициализация основного макета
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(placeholderLabel_);
    mainLayout->addWidget(contentWidget_);

    this->setVisible(false);
}

void PropertiesWatcher::initButton(QPushButton *button, const QString &text) noexcept
{
    assert(button != nullptr);
    button->setText(text);
    button->setFocusPolicy(Qt::NoFocus);
}

void PropertiesWatcher::clear(bool hideContent) noexcept
{
    if (hideContent) {
        contentWidget_->setVisible(false);
        placeholderLabel_->setVisible(true);
    }

    const auto *selectionModel = treeView_->selectionModel();
    assert(selectionModel != nullptr);
    disconnect(selectionModel, &QItemSelectionModel::selectionChanged, this, 0);

    framedObjectModel_->clear();
    metaPropertyModel_->clear();
}

void PropertiesWatcher::setFramedRootObjectData(const QStandardItemModel model, const MetaPropertyData rootMetaData) noexcept
{
    clear(false);

    const auto *sourceRoot = model.invisibleRootItem();
    auto *targetRoot = framedObjectModel_->invisibleRootItem();
    cloneModelItems(sourceRoot, targetRoot);
    setMetaPropertyModel(std::move(rootMetaData));

    assert(framedObjectModel_->rowCount() == 1);
    const auto *rootItem = framedObjectModel_->item(0);
    assert(rootItem != nullptr);

    const auto rootIndex = framedObjectModel_->indexFromItem(rootItem);
    assert(rootIndex.isValid());
    treeView_->setCurrentIndex(rootIndex);
    if (rootItem->rowCount() != 0) {
        treeView_->expand(rootIndex);
    }

    const auto *selectionModel = treeView_->selectionModel();
    assert(selectionModel != nullptr);
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this,
            &PropertiesWatcher::framedSelectionChanged);

    contentWidget_->setVisible(true);
    placeholderLabel_->setVisible(false);
}

void PropertiesWatcher::setMetaPropertyModel(const MetaPropertyData metaData) noexcept
{
    metaPropertyModel_->clear();

    // При очищении модели эти настройки пропадают, поэтому при каждом обновлении
    // выставляем их заново.
    metaPropertyModel_->setHorizontalHeaderLabels({ "Property", "Value" });
    tableView_->setColumnWidth(0, tableView_->viewport()->width() / 3);
    tableView_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    tableView_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    //! TODO: Изначально идея была в том, что если propertyValue - пустой,
    //! то значит нам не удалось преобразовать QVariant в нужное значение,
    //! и, соответственно, мы не можем допустить проверку таких свойств.
    //! Но по факту QVariant может быть просто пустой строкой, в связи с
    //! чем пока не делаем эти строки "неактивными", и не переносим их
    //! вниз.
    //! std::vector<TableRow> rowsWithKnownValues;
    //! std::vector<TableRow> rowsWithUnknownValues;
    using TableRow = std::pair<QStandardItem *, QStandardItem *>;
    std::vector<TableRow> rows;

    for (const auto &pair : metaData) {
        const auto valueisKnown = !pair.second.isEmpty();
        auto *nameItem = new QStandardItem(pair.first);
        auto *valueItem = new QStandardItem(valueisKnown ? pair.second : QStringLiteral("<empty>"));
        rows.push_back({ nameItem, valueItem });
        //! if (valueisKnown) {
        //!     rowsWithKnownValues.push_back({ nameItem, valueItem });
        //! }
        //! else {
        //!     nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsSelectable);
        //!     valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsSelectable);
        //!     rowsWithUnknownValues.push_back({ nameItem, valueItem });
        //! }
    }

    auto sortFunc = [](const TableRow &a, const TableRow &b) { return a.first->text() < b.first->text(); };
    std::sort(rows.begin(), rows.end(), sortFunc);
    for (auto &row : rows) {
        metaPropertyModel_->appendRow({ row.first, row.second });
    }
}

void PropertiesWatcher::framedSelectionChanged(const QItemSelection &selected,
                                               const QItemSelection &deselected) noexcept
{
    Q_UNUSED(deselected);
    if (selected.isEmpty()) {
        return;
    }
    const auto index = selected.indexes().first();
    assert(index.isValid());
    emit inprocessController_->requestFramedObjectChange(getItemPath(std::move(index)));
}

void PropertiesWatcher::metaSelectionChanged() noexcept
{
    acceptSelectionButton_->setEnabled(tableView_->selectionModel()->hasSelection());
}

void PropertiesWatcher::acceptSelection() noexcept
{
    const auto selectedObjectes = treeView_->selectionModel()->selectedIndexes();
    assert(selectedObjectes.size() == 1);
    const auto selectedObjectPath = selectedObjectes.first().data(Qt::UserRole).value<QString>();
    assert(!selectedObjectPath.isEmpty());

    const auto selectedPropertyRows = tableView_->selectionModel()->selectedRows();
    assert(selectedPropertyRows.size() > 0);

    std::vector<std::pair<QString, QString>> result;
    for (const auto &index : selectedPropertyRows) {
        const auto rowIndex = index.row();
        const auto property = metaPropertyModel_->index(rowIndex, 0).data().toString();
        assert(!property.isEmpty());
        const auto value = metaPropertyModel_->index(rowIndex, 1).data().toString();
        assert(!value.isEmpty());

        result.emplace_back(property, value);
    }

    tableView_->clearSelection();

    emit inprocessController_->newMetaPropertyVerification(std::move(selectedObjectPath), std::move(result));
}
} // namespace QtAda::core::gui
