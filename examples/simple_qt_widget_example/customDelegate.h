#pragma once

#include <QStyledItemDelegate>
#include <QPainter>

class CustomItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    CustomItemDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);

        painter->save();
        painter->fillRect(options.rect, index.data(Qt::BackgroundRole).value<QColor>());
        painter->restore();

        QStyledItemDelegate::paint(painter, options, index);
    }
};
