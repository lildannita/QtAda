#include "FilterUtils.hpp"

#include <QEvent>
#include <QString>
#include <QWidget>
#include <QModelIndex>

#include <QComboBox>

//! TODO: remove
#include <iostream>

namespace QtAda::core::utils {
static const std::pair<Qt::MouseButton, QLatin1String> mouseButtons[] = {
    { Qt::NoButton, QLatin1String("Qt::NoButton") },
    { Qt::LeftButton, QLatin1String("Qt::LeftButton") },
    { Qt::RightButton, QLatin1String("Qt::RightButton") },
    { Qt::MiddleButton, QLatin1String("Qt::MiddleButton") },
    { Qt::BackButton, QLatin1String("Qt::BackButton") },
    { Qt::ForwardButton, QLatin1String("Qt::ForwardButton") },
};

static const std::map<WidgetClass, QVector<QLatin1String>> widgetClasses = {
    { ComboBox, { QLatin1String("QComboBox") } },
    { SpinBox, { QLatin1String("QSpinBox"), QLatin1String("QDoubleSpinBox") } },
    { CheckBox, { QLatin1String("QCheckBox") } },
    { Button, { QLatin1String("QAbstractButton") } },
};

static uint metaObjectIndexInObjectList(const QObject *obj, const QObjectList &children) noexcept
{
    assert(!children.isEmpty());

    const auto className = obj->metaObject()->className();
    uint index = 0;

    auto it = std::find_if(children.begin(), children.end(), [&](const QObject *item) {
        if (item == obj) {
            return true;
        }
        if (className == item->metaObject()->className())
            index++;
        return false;
    });

    assert(it != children.end());
    return index;
}

static uint objectIndexInObjectList(const QObject *obj, const QObjectList &children) noexcept
{
    assert(!children.isEmpty());

    const auto objName = obj->objectName();
    uint index = 0;

    auto it = std::find_if(children.begin(), children.end(), [&](const QObject *item) {
        bool sameName = objName == item->objectName();
        if (item == obj) {
            return true;
        }
        if (objName == item->objectName())
            index++;
        return false;
    });

    assert(it != children.end());
    return index;
}

static QString metaObjectId(const QObject *obj) noexcept
{
    const QString metaObjName = obj->metaObject()->className();
    const auto *parent = obj->parent();
    return QString("%1_%2")
        .arg(metaObjName)
        .arg(parent ? metaObjectIndexInObjectList(obj, parent->children()) : 0);
}

static QString objectId(const QObject *obj) noexcept
{
    const QString objName = obj->objectName();
    const auto *parent = obj->parent();
    return QString("%1_%2").arg(objName).arg(
        parent ? objectIndexInObjectList(obj, parent->children()) : 0);
}

QString objectPath(const QObject *obj) noexcept
{
    QStringList pathComponents;
    for (const QObject *current = obj; current != nullptr; current = current->parent()) {
        QString identifier = current->objectName().isEmpty()
                                 ? QString("c=%1").arg(metaObjectId(current))
                                 : QString("n=%1").arg(objectId(current));
        pathComponents.prepend(identifier);
    }
    return pathComponents.join('/');
}

QString mouseButtonToString(Qt::MouseButton mouseButton) noexcept
{
    for (const auto &pair : mouseButtons) {
        if (pair.first == mouseButton) {
            return pair.second;
        }
    }
    return QLatin1String("<unknown>");
}

bool mouseEventCanBeFiltered(QWidget *widget, QEvent *event) noexcept
{
    const auto type = event->type();
    return widget != nullptr && event != nullptr
           && (type == QEvent::MouseButtonRelease || type == QEvent::MouseButtonDblClick);
}

std::pair<QWidget *, size_t>
searchSpecificWidgetWithIteration(QWidget *widget, WidgetClass widgetClass, size_t limit) noexcept
{
    const auto &classNames = widgetClasses.at(widgetClass);
    for (size_t i = 0; i < limit && widget != nullptr; i++) {
        const auto *metaObject = widget->metaObject();
        while (metaObject != nullptr
               && !classNames.contains(QLatin1String(metaObject->className()))) {
            metaObject = metaObject->superClass();
        }

        if (metaObject != nullptr) {
            return std::make_pair(widget, i);
        }
        else {
            widget = static_cast<QWidget *>(widget->parent());
        }
    }
    return std::make_pair(nullptr, 0);
}

QWidget *searchSpecificWidget(QWidget *widget, WidgetClass widgetClass, size_t limit) noexcept
{
    return searchSpecificWidgetWithIteration(widget, widgetClass, limit).first;
}

QString itemIdInWidgetView(QWidget *widget, QModelIndex index, WidgetClass widgetClass) noexcept
{
    if (widget == nullptr) {
        return QString();
    }

    switch (widgetClass) {
    case ComboBox: {
        const auto *comboBox = qobject_cast<QComboBox *>(widget);
        if (comboBox == nullptr) {
            return QString();
        }
        const auto itemText = comboBox->itemText(index.row());
        const auto itemsCount = comboBox->count();
        size_t itemIndex = 0;
        for (size_t i = 0; i < itemsCount; i++) {
            if (i == index.row()) {
                return QStringLiteral("%1_%2").arg(itemText).arg(itemIndex);
            }
            if (itemText == comboBox->itemText(i)) {
                itemIndex++;
            }
        }
        Q_UNREACHABLE();
    }
    default:
        return QString();
    }
}
} // namespace QtAda::core::utils
