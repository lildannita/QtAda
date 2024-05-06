#pragma once

#include <QWidget>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QPushButton;
class QTreeView;
class QTableView;
class QItemSelection;
class QStandardItemModel;
class QLabel;
class QStandardItem;
QT_END_NAMESPACE

namespace QtAda::inprocess {
class InprocessController;

class PropertiesWatcher final : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesWatcher(InprocessController *inprocessController, QWidget *parent) noexcept;

signals:
    void newMetaPropertyVerification(const QString &objectPath,
                                     const std::vector<std::pair<QString, QString>> &verifications);

public slots:
    void clear(bool hideContent = true) noexcept;

private slots:
    void setFramedRootObjectData(const QVariantMap &model,
                                 const QList<QVariantMap> &rootMetaData) noexcept;
    void setMetaPropertyModel(const QList<QVariantMap> &metaData) noexcept;

    void framedSelectionChanged(const QItemSelection &selected,
                                const QItemSelection &deselected) noexcept;
    void metaSelectionChanged() noexcept;
    void acceptSelection() noexcept;

private:
    InprocessController *inprocessController_ = nullptr;

    QLabel *placeholderLabel_ = nullptr;
    QWidget *contentWidget_ = nullptr;
    QPushButton *acceptSelectionButton_ = nullptr;

    QTreeView *treeView_ = nullptr;
    QStandardItemModel *framedObjectModel_ = nullptr;
    QTableView *tableView_ = nullptr;
    QStandardItemModel *metaPropertyModel_ = nullptr;

    QPushButton *initButton(const QString &text) noexcept;
    void fillTreeModel(QStandardItem *parentItem, const QVariantMap &model);
};
} // namespace QtAda::inprocess
