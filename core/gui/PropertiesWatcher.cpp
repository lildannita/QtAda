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
static QModelIndex findModelIndexForObject(const QAbstractItemModel *model,
                                           const QObject *targetObject,
                                           const QModelIndex &parent = QModelIndex())
{
    int rowCount = model->rowCount(parent);
    int columnCount = model->columnCount(parent);

    for (int row = 0; row < rowCount; ++row) {
        for (int column = 0; column < columnCount; ++column) {
            QModelIndex index = model->index(row, column, parent);
            QVariant data = model->data(index, Qt::UserRole);

            QObject *storedObject = qvariant_cast<QObject *>(data);
            if (storedObject == targetObject) {
                return index;
            }

            if (model->hasChildren(index)) {
                QModelIndex foundIndex = findModelIndexForObject(model, targetObject, index);
                if (foundIndex.isValid()) {
                    return foundIndex;
                }
            }
        }
    }

    return QModelIndex();
}

PropertiesWatcher::PropertiesWatcher(QWidget *parent) noexcept
    : QWidget{ parent }
    , selectAll{ new QPushButton }
    , clearSelection{ new QPushButton }
    , acceptSelection{ new QPushButton }
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

    // Инициализация растягивающегося макета под View-компоненты
    QSplitter *viewsSplitter = new QSplitter(Qt::Vertical);
    viewsSplitter->setChildrenCollapsible(false);
    viewsSplitter->addWidget(treeView_);
    viewsSplitter->addWidget(tableView_);

    // Инициализация кнопок, управляющих выбором свойств инспектируемого объекта
    initButton(selectAll, "Select All");
    initButton(clearSelection, "Clear Selection");
    initButton(acceptSelection, "Accept Selection");
    // Инициализация макета с кнопками
    QWidget *buttonsWidget = new QWidget;
    QHBoxLayout *buttonsLayout = new QHBoxLayout(buttonsWidget);
    buttonsLayout->addWidget(selectAll);
    buttonsLayout->addWidget(clearSelection);
    buttonsLayout->addWidget(generateSeparator(this));
    buttonsLayout->addWidget(acceptSelection);

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

void PropertiesWatcher::clear(bool clearOnlyModel) noexcept
{
    if (clearOnlyModel) {
        contentWidget_->setVisible(false);
        placeholderLabel_->setVisible(true);
    }

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

void PropertiesWatcher::handleTreeModelUpdated(
    std::optional<const QModelIndex> prefferedCurrentIndex) noexcept
{
    assert(framedObjectModel_ != nullptr);
    assert(framedObjectModel_->rowCount() == 1);
    assert(treeView_ != nullptr);

    const auto *rootItem = framedObjectModel_->item(0);
    assert(rootItem != nullptr);

    const auto rootIndex = framedObjectModel_->indexFromItem(rootItem);
    assert(rootIndex.isValid());
    if (prefferedCurrentIndex.has_value() && prefferedCurrentIndex->isValid()) {
        treeView_->setCurrentIndex(*prefferedCurrentIndex);
    }
    else {
        treeView_->setCurrentIndex(rootIndex);
    }
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

void PropertiesWatcher::handleObjectCreation(const QObject *obj) noexcept
{
    if (obj == contentWidget_) {
        assert(true);
    }

    if (framedObjectModel_->rowCount() == 0) {
        return;
    }

    const auto *rootItem = framedObjectModel_->item(0);
    assert(rootItem != nullptr);
    const auto *rootObject = rootItem->data(Qt::UserRole).value<QObject *>();

    const auto selectedIndexes = treeView_->selectionModel()->selectedIndexes();
    if (selectedIndexes.size() != 1) {
        clear(true);
        addFramedObjectToModel(rootObject, nullptr);
        updateMetaPropertyModel(rootObject);
        handleTreeModelUpdated();
    }
    else {
        const auto *selectedObject = selectedIndexes.first().data(Qt::UserRole).value<QObject *>();
        clear(true);
        addFramedObjectToModel(rootObject, nullptr);
        updateMetaPropertyModel(selectedObject);
        handleTreeModelUpdated(findModelIndexForObject(framedObjectModel_, selectedObject));
    }
}

void PropertiesWatcher::handleObjectDestruction(const QObject *obj) noexcept
{
    if (obj == contentWidget_) {
        assert(false);
    }

    if (framedObjectModel_->rowCount() == 0) {
        return;
    }

    const auto *rootItem = framedObjectModel_->item(0);
    assert(rootItem != nullptr);
    const auto *rootObject = rootItem->data(Qt::UserRole).value<QObject *>();

    if (obj == rootObject) {
        clear();
        return;
    }

    const auto selectedIndexes = treeView_->selectionModel()->selectedIndexes();
    if (selectedIndexes.size() != 1) {
        clear(true);
        addFramedObjectToModel(rootObject, nullptr);
        updateMetaPropertyModel(rootObject);
        handleTreeModelUpdated();
    }
    else {
        const auto *selectedObject = selectedIndexes.first().data(Qt::UserRole).value<QObject *>();
        clear(true);
        addFramedObjectToModel(rootObject, nullptr);
        if (obj == selectedObject) {
            updateMetaPropertyModel(rootObject);
            handleTreeModelUpdated();
        }
        else {
            updateMetaPropertyModel(selectedObject);
            handleTreeModelUpdated(findModelIndexForObject(framedObjectModel_, selectedObject));
        }
    }
}
} // namespace QtAda::core::gui
