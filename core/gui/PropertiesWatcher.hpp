#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
class QTreeView;
class QTableView;
class QStandardItemModel;
class QStandardItem;
class QItemSelection;
class QLabel;
QT_END_NAMESPACE

namespace QtAda::core::gui {
class PropertiesWatcher : public QWidget {
    Q_OBJECT
public:
    PropertiesWatcher(QWidget *parent) noexcept;

    void clear() noexcept;

signals:
    void framedObjectChangedFromWatcher(QObject *framedObject);
    void newMetaPropertyVerification(const QObject *object,
                                     const std::vector<std::pair<QString, QString>> &verifications);

public slots:
    void setFramedObject(const QObject *object) noexcept;
    void setFrame(const QObject *frame) noexcept
    {
        frame_ = frame;
    }
    void removeFrame() noexcept
    {
        frame_ = nullptr;
        clear();
    }

private slots:
    void framedSelectionChanged(const QItemSelection &selected,
                                const QItemSelection &deselected) noexcept;
    void metaSelectionChanged() noexcept;
    void acceptSelection() noexcept;

private:
    QWidget *contentWidget_ = nullptr;
    QLabel *placeholderLabel_ = nullptr;

    QTreeView *treeView_ = nullptr;
    QStandardItemModel *framedObjectModel_ = nullptr;
    const QObject *frame_ = nullptr;
    QTableView *tableView_ = nullptr;
    QStandardItemModel *metaPropertyModel_ = nullptr;

    QPushButton *acceptSelectionButton_ = nullptr;

    void addFramedObjectToModel(const QObject *object, QStandardItem *parentViewItem) noexcept;
    void updateMetaPropertyModel(const QObject *object) noexcept;
    void handleTreeModelUpdated() noexcept;

    void initButton(QPushButton *button, const QString &text) noexcept;
};
} // namespace QtAda::core::gui
