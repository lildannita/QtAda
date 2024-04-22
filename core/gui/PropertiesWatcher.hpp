#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
class QTreeView;
class QStandardItemModel;
class QStandardItem;
class QItemSelection;
QT_END_NAMESPACE

namespace QtAda::core::gui {
class PropertiesWatcher : public QWidget {
    Q_OBJECT
public:
    PropertiesWatcher(QWidget *parent) noexcept;

    void clear() noexcept;

signals:
    void framedObjectChangedFromWatcher(QObject *framedObject);

public slots:
    void setFramedObject(const QObject *object) noexcept;
    void setFrame(const QObject *frame) noexcept
    {
        frame_ = frame;
    }
    void removeFrame() noexcept
    {
        frame_ = nullptr;
    }

private slots:
    void framedSelectionChanged(const QItemSelection &selected,
                                const QItemSelection &deselected) noexcept;

private:
    QPushButton *selectAll = nullptr;
    QPushButton *clearSelection = nullptr;
    QPushButton *acceptSelection = nullptr;

    QTreeView *treeView_ = nullptr;
    QStandardItemModel *framedObjectModel_ = nullptr;
    const QObject *frame_ = nullptr;

    void addFramedObjectToModel(const QObject *object, QStandardItem *parentViewItem) noexcept;
    void handleTreeModelUpdated() noexcept;

    void initButton(QPushButton *button, const QString &text) noexcept;
};
} // namespace QtAda::core::gui
