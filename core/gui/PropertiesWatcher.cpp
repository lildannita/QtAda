#include "PropertiesWatcher.hpp"

#include <QQuickItem>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTreeView>

#include "utils/Tools.hpp"
#include "GuiTools.hpp"

namespace QtAda::core::gui {
PropertiesWatcher::PropertiesWatcher(QWidget *parent) noexcept
    : QWidget{ parent }
    , selectAll{ new QPushButton }
    , clearSelection{ new QPushButton }
    , acceptSelection{ new QPushButton }
    , framedObjectModel_{ new QStandardItemModel }
    , treeView_{ new QTreeView }
{
    // Инициализация QTreeView, отображающего дерево элементов
    treeView_->setHeaderHidden(true);
    treeView_->setModel(framedObjectModel_);

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

    // Инициализация основного макета
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(treeView_);
    mainLayout->addWidget(buttonsWidget);

    this->setVisible(false);
}

void PropertiesWatcher::clear() noexcept
{
    assert(framedObjectModel_ != nullptr);
    framedObjectModel_->clear();

    const auto *selectionModel = treeView_->selectionModel();
    assert(selectionModel != nullptr);
    disconnect(selectionModel, &QItemSelectionModel::selectionChanged, this, 0);
}

void PropertiesWatcher::setFramedObject(const QObject *object) noexcept
{
    clear();
    addFramedObjectToModel(object, nullptr);
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

    if (qobject_cast<QWidget *>(selectedObject) == nullptr
        && qobject_cast<QQuickItem *>(selectedObject) == nullptr) {
        //! TODO: В будущем, когда будет решено, что делать с "не-GUI" компонентами,
        //! нужно будет пересмотреть эту проверку.
        return;
    }

    emit framedObjectChangedFromWatcher(selectedObject);
}
} // namespace QtAda::core::gui
