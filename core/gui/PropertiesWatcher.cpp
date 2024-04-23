#include "PropertiesWatcher.hpp"

#include <QQuickItem>
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

#include "utils/Tools.hpp"
#include "GuiTools.hpp"

namespace QtAda::core::gui {
PropertiesWatcher::PropertiesWatcher(QWidget *parent) noexcept
    : QWidget{ parent }
    , acceptSelectionButton_{ new QPushButton }
    , treeView_{ new QTreeView }
    , framedObjectModel_{ new QStandardItemModel }
    , tableView_{ new QTableView }
    , metaPropertyModel_{ new QStandardItemModel }
    , contentWidget_{ new QWidget }
    , placeholderLabel_{ new QLabel }
{
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

void PropertiesWatcher::clear() noexcept
{
    contentWidget_->setVisible(false);
    placeholderLabel_->setVisible(true);

    assert(framedObjectModel_ != nullptr);
    framedObjectModel_->clear();
    assert(metaPropertyModel_ != nullptr);
    metaPropertyModel_->clear();

    const auto *selectionModel = treeView_->selectionModel();
    assert(selectionModel != nullptr);
    disconnect(selectionModel, &QItemSelectionModel::selectionChanged, this, 0);
}

void PropertiesWatcher::setFramedObject(const QObject *object) noexcept
{
    clear();
    addFramedObjectToModel(object, nullptr);
    updateMetaPropertyModel(object);
    handleTreeModelUpdated();
}

void PropertiesWatcher::addFramedObjectToModel(const QObject *object,
                                               QStandardItem *parentViewItem) noexcept
{
    assert(object != nullptr);
    if (object == frame_
        || (qobject_cast<const QWidget *>(object) == nullptr
            && qobject_cast<const QQuickItem *>(object) == nullptr)) {
        //! TODO: возможно, лучше убрать проверку на каст, так как
        //! некоторые элементы в GUI могут быть не QWidget и не QQuickItem.
        return;
    }

    const auto objectName = object->objectName();
    const auto viewItemName
        = QStringLiteral("%1 (%2)")
              .arg(objectName.isEmpty() ? tools::pointerToString(object) : objectName)
              .arg(object->metaObject()->className());
    auto *viewItem = new QStandardItem(viewItemName);
    viewItem->setData(QVariant::fromValue(const_cast<QObject *>(object)), Qt::UserRole);
    if (parentViewItem == nullptr) {
        assert(framedObjectModel_ != nullptr);
        framedObjectModel_->appendRow(viewItem);
    }
    else {
        parentViewItem->appendRow(viewItem);
    }

    for (const auto *child : object->children()) {
        addFramedObjectToModel(child, viewItem);
    }
}

void PropertiesWatcher::initButton(QPushButton *button, const QString &text) noexcept
{
    assert(button != nullptr);
    button->setText(text);
    button->setFocusPolicy(Qt::NoFocus);
}

void PropertiesWatcher::handleTreeModelUpdated() noexcept
{
    assert(framedObjectModel_ != nullptr);
    assert(framedObjectModel_->rowCount() == 1);
    assert(treeView_ != nullptr);

    const auto *rootItem = framedObjectModel_->item(0);
    assert(rootItem != nullptr);

    const auto index = framedObjectModel_->indexFromItem(rootItem);
    treeView_->setCurrentIndex(index);
    if (rootItem->rowCount() != 0) {
        treeView_->expand(index);
    }

    const auto *selectionModel = treeView_->selectionModel();
    assert(selectionModel != nullptr);
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this,
            &PropertiesWatcher::framedSelectionChanged);

    contentWidget_->setVisible(true);
    placeholderLabel_->setVisible(false);
}

void PropertiesWatcher::updateMetaPropertyModel(const QObject *object) noexcept
{
    assert(metaPropertyModel_ != nullptr);
    metaPropertyModel_->clear();

    // При очищении модели эти настройки пропадают, поэтому при каждом обновлении
    // выставляем их заново.
    metaPropertyModel_->setHorizontalHeaderLabels({ "Property", "Value" });
    tableView_->setColumnWidth(0, tableView_->viewport()->width() / 3);
    tableView_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    tableView_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    assert(object != nullptr);
    const auto *metaObject = object->metaObject();
    assert(metaObject != nullptr);

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

    const auto propertyCount = metaObject->propertyCount();
    for (int i = 0; i < propertyCount; i++) {
        const auto metaProperty = metaObject->property(i);
        assert(metaProperty.isValid());
        const auto propertyValue = tools::metaPropertyValueToString(object, metaProperty);
        const auto valueisKnown = !propertyValue.isEmpty();

        auto *nameItem = new QStandardItem(metaProperty.name());
        auto *valueItem
            = new QStandardItem(valueisKnown ? propertyValue : QStringLiteral("<empty>"));
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

    auto *selectedObject = index.data(Qt::UserRole).value<QObject *>();
    assert(selectedObject != nullptr);
    updateMetaPropertyModel(selectedObject);

    if (qobject_cast<QWidget *>(selectedObject) == nullptr
        && qobject_cast<QQuickItem *>(selectedObject) == nullptr) {
        //! TODO: В будущем, когда будет решено, что делать с "не-GUI" компонентами,
        //! нужно будет пересмотреть эту проверку.
        return;
    }

    emit framedObjectChangedFromWatcher(selectedObject);
}

void PropertiesWatcher::metaSelectionChanged() noexcept
{
    assert(tableView_ != nullptr);
    assert(acceptSelectionButton_ != nullptr);

    acceptSelectionButton_->setEnabled(tableView_->selectionModel()->hasSelection());
}

void PropertiesWatcher::acceptSelection() noexcept
{
    const auto selectedObjectes = treeView_->selectionModel()->selectedIndexes();
    assert(selectedObjectes.size() == 1);
    const auto selectedObject = selectedObjectes.first().data(Qt::UserRole).value<QObject *>();
    assert(selectedObject != nullptr);

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

    emit newMetaPropertyVerification(selectedObject, std::move(result));
}
} // namespace QtAda::core::gui
