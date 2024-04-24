#pragma once

#include <QWidget>
#include <deque>

#include "InprocessController.hpp"

QT_BEGIN_NAMESPACE
class QPushButton;
class QTreeView;
class QTableView;
class QItemSelection;
class QStandardItemModel;
class QLabel;
QT_END_NAMESPACE

namespace QtAda::inprocess {
using MetaPropertyData = std::vector<std::pair<QString, QString>>;

class PropertiesWatcher : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesWatcher(InprocessController *inprocessController, QWidget *parent) noexcept;

signals:
    //! TODO: нужно будет подключить к ScriptWriter, когда я его перенесу
    void newMetaPropertyVerification(const QString &objectPath, const MetaPropertyData &verifications);

public slots:
    void clear(bool hideContent = true) noexcept;

private slots:
    void setFramedRootObjectData(const QStandardItemModel model, const MetaPropertyData rootMetaData) noexcept;
    void setMetaPropertyModel(const MetaPropertyData metaData) noexcept;

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

    void initButton(QPushButton *button, const QString &text) noexcept;
};
} // namespace QtAda::core::gui
