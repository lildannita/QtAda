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
#include <QVariantMap>
#include <QSplitter>
#include <QLabel>

#include "InprocessTools.hpp"
#include "InprocessController.hpp"

namespace QtAda::inprocess {
static QList<int> getItemIndexPath(const QModelIndex &index)
{
    QList<int> path;
    QModelIndex current = index;
    while (current.isValid()) {
        path.push_front(current.row());
        current = current.parent();
    }
    return path;
}

PropertiesWatcher::PropertiesWatcher(InprocessController *inprocessController,
                                     QWidget *parent) noexcept
    : QWidget{ parent }
    , inprocessController_{ inprocessController }
    , treeView_{ new QTreeView(this) }
    , framedObjectModel_{ new QStandardItemModel(this) }
    , tableView_{ new QTableView }
    , metaPropertyModel_{ new QStandardItemModel(this) }
    , contentWidget_{ new QWidget(this) }
    , placeholderLabel_{ new QLabel(this) }
{
    connect(inprocessController_, &InprocessController::newFramedRootObjectData, this,
            &PropertiesWatcher::setFramedRootObjectData);
    connect(inprocessController_, &InprocessController::newMetaPropertyData, this,
            &PropertiesWatcher::setMetaPropertyModel);

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
    auto *viewsSplitter = new QSplitter(Qt::Vertical);
    viewsSplitter->setChildrenCollapsible(false);
    viewsSplitter->addWidget(treeView_);
    viewsSplitter->addWidget(tableView_);

    // Инициализация кнопок, управляющих выбором свойств инспектируемого объекта
    auto *selectAllButton = initButton("Select All");
    auto *clearSelectionButton = initButton("Clear Selection");
    acceptSelectionButton_ = initButton("Accept Selection");
    connect(selectAllButton, &QPushButton::clicked, tableView_, &QAbstractItemView::selectAll);
    connect(clearSelectionButton, &QPushButton::clicked, tableView_,
            &QAbstractItemView::clearSelection);
    connect(acceptSelectionButton_, &QPushButton::clicked, this,
            &PropertiesWatcher::acceptSelection);
    acceptSelectionButton_->setEnabled(false);

    // Инициализация макета с кнопками
    auto *buttonsWidget = new QWidget;
    auto *buttonsLayout = new QHBoxLayout(buttonsWidget);
    buttonsLayout->addWidget(selectAllButton);
    buttonsLayout->addWidget(clearSelectionButton);
    buttonsLayout->addWidget(tools::initSeparator(this));
    buttonsLayout->addWidget(acceptSelectionButton_);

    // Инициализация макета под основные компоненты
    auto *contentLayout = new QVBoxLayout(contentWidget_);
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
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(placeholderLabel_);
    mainLayout->addWidget(contentWidget_);

    this->setVisible(false);
}

QPushButton *PropertiesWatcher::initButton(const QString &text) noexcept
{
    auto *button = new QPushButton(this);
    button->setText(text);
    button->setFocusPolicy(Qt::NoFocus);
    return button;
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

void PropertiesWatcher::setFramedRootObjectData(const QVariantMap &model,
                                                const QList<QVariantMap> &rootMetaData) noexcept
{
    clear(false);

    fillTreeModel(nullptr, model);
    setMetaPropertyModel(rootMetaData);

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

void PropertiesWatcher::setMetaPropertyModel(const QList<QVariantMap> &metaData) noexcept
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

    for (const auto &rowData : metaData) {
        const auto property = rowData["property"].toString();
        const auto value = rowData["value"].toString();

        const auto valueisKnown = !value.isEmpty();
        auto *nameItem = new QStandardItem(property);
        auto *valueItem = new QStandardItem(valueisKnown ? value : QStringLiteral("<empty>"));
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

    auto sortFunc
        = [](const TableRow &a, const TableRow &b) { return a.first->text() < b.first->text(); };
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
    emit inprocessController_->requestFramedObjectChange(getItemIndexPath(std::move(index)));
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

    emit newMetaPropertyVerification(std::move(selectedObjectPath), std::move(result));
}

void PropertiesWatcher::fillTreeModel(QStandardItem *parentItem, const QVariantMap &model)
{
    auto *item = new QStandardItem(model["object"].toString());
    item->setData(model["path"], Qt::UserRole);

    if (parentItem) {
        parentItem->appendRow(item);
    }
    else {
        framedObjectModel_->appendRow(item);
    }

    if (model.contains("children")) {
        assert(model["children"].canConvert<QVariantList>());
        const auto children = model["children"].toList();
        for (const auto &child : children) {
            fillTreeModel(item, child.toMap());
        }
    }
}
} // namespace QtAda::inprocess
