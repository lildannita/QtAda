#include "PropertiesWatcher.hpp"

#include <QQuickItem>
#include <QStandardItem>
#include <QStandardItemModel>
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
    , selectedObjectModel_{ new QStandardItemModel }
    , treeView_{ new QTreeView }
{
    // Инициализация QTreeView, отображающего дерево элементов
    treeView_->setHeaderHidden(true);
    treeView_->setModel(selectedObjectModel_);

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
    assert(selectedObjectModel_ != nullptr);
    selectedObjectModel_->clear();
}

void PropertiesWatcher::setSelectedObject(const QObject *object) noexcept
{
    clear();
    addObjectToModel(object, nullptr);
    handleTreeModelUpdated();
}

void PropertiesWatcher::addObjectToModel(const QObject *object,
                                         QStandardItem *parentViewItem) noexcept
{
    assert(object != nullptr);
    if (object == frame_
        || (qobject_cast<const QWidget *>(object) == nullptr
            && qobject_cast<const QQuickItem *>(object) == nullptr)) {
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
        assert(selectedObjectModel_ != nullptr);
        selectedObjectModel_->appendRow(viewItem);
    }
    else {
        parentViewItem->appendRow(viewItem);
    }

    for (const auto *child : object->children()) {
        addObjectToModel(child, viewItem);
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
    assert(selectedObjectModel_ != nullptr);
    assert(selectedObjectModel_->rowCount() == 1);
    assert(treeView_ != nullptr);

    const auto *rootItem = selectedObjectModel_->item(0);
    assert(rootItem != nullptr);

    const auto index = selectedObjectModel_->indexFromItem(rootItem);
    treeView_->setCurrentIndex(index);
    if (rootItem->rowCount() != 0) {
        treeView_->expand(index);
    }
}
} // namespace QtAda::core::gui
