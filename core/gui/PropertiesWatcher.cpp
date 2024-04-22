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
    initButton(selectAll, "Select All");
    initButton(clearSelection, "Clear Selection");
    initButton(acceptSelection, "Accept Selection");

    QWidget *buttonsWidget = new QWidget;
    QHBoxLayout *buttonsLayout = new QHBoxLayout(buttonsWidget);
    buttonsLayout->addWidget(selectAll);
    buttonsLayout->addWidget(clearSelection);
    buttonsLayout->addWidget(generateSeparator(this));
    buttonsLayout->addWidget(acceptSelection);

    treeView_->setHeaderHidden(true);
    treeView_->setModel(selectedObjectModel_);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(treeView_);
    mainLayout->addWidget(buttonsWidget);

    this->setVisible(false);
}

void PropertiesWatcher::setSelectedObject(const QObject *object) noexcept
{
    assert(selectedObjectModel_ != nullptr);
    selectedObjectModel_->clear();
    addObjectToModel(object, nullptr);
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
        initFirstViewItem(viewItem);
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

void PropertiesWatcher::initFirstViewItem(QStandardItem *viewItem) noexcept
{
    assert(viewItem != nullptr);
    assert(selectedObjectModel_ != nullptr);
    const auto index = selectedObjectModel_->indexFromItem(viewItem);
    treeView_->expand(index);
}
} // namespace QtAda::core::gui
