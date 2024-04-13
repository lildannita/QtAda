#include "QuickEventFilter.hpp"

#include <QQmlProperty>

//! TODO: убрать
#include <iostream>

namespace QtAda::core::filters {
// Принцип построения этого std::map смотри в WidgetEventFilter.cpp
static const std::map<QuickClass, std::pair<QLatin1String, size_t>> s_quickMetaMap = {
    { QuickClass::Button, { QLatin1String("QQuickButton"), 1 } },
    { QuickClass::MouseArea, { QLatin1String("QQuickMouseArea"), 1 } },
};

static const std::vector<QuickClass> s_processedTextWidgets = {};

//! TODO: нужна ли обработка зажатия кастомной кнопки?
static QString qButtonFilter(const QQuickItem *item, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::Button));
    if (item == nullptr) {
        return QString();
    }

    const auto buttonRect = item->boundingRect();
    const auto buttonText = QQmlProperty::read(item, "text").toString();
    const auto clickPos = item->mapFromGlobal(event->globalPos());

    return QStringLiteral("button%1('%2');%3")
        .arg(buttonRect.contains(clickPos)
                 ? (event->type() == QEvent::MouseButtonDblClick ? "DblClick" : "Click")
                 : "Press")
        .arg(utils::objectPath(item))
        .arg(buttonText.isEmpty() ? "" : QStringLiteral(" // Button text: '%1'").arg(buttonText));
}

static QString qMouseAreaFilter(const QQuickItem *item, const QMouseEvent *event) noexcept
{
    if (!utils::mouseEventCanBeFiltered(item, event)) {
        return QString();
    }

    item = utils::searchSpecificComponent(item, s_quickMetaMap.at(QuickClass::MouseArea));
    if (item == nullptr) {
        return QString();
    }

    const auto mouseAreaRect = item->boundingRect();
    const auto clickPos = item->mapFromGlobal(event->globalPos());
    return QStringLiteral("mouseArea%1('%2');")
        .arg(mouseAreaRect.contains(clickPos)
                 ? (event->type() == QEvent::MouseButtonDblClick ? "DblClick" : "Click")
                 : "Press")
        .arg(utils::objectPath(item));
}
} // namespace QtAda::core::filters

namespace QtAda::core {
QuickEventFilter::QuickEventFilter(QObject *parent) noexcept
{
    mouseFilters_ = {
        filters::qButtonFilter,
        filters::qMouseAreaFilter,
    };
}

QString QuickEventFilter::callMouseFilters(const QObject *obj, const QEvent *event,
                                           bool isContinuous, bool isSpecialEvent) noexcept
{
    auto *item = qobject_cast<const QQuickItem *>(obj);
    if (item == nullptr) {
        return QString();
    }

    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (mouseEvent == nullptr) {
        return QString();
    }

    for (auto &filter : mouseFilters_) {
        const auto result = filter(item, mouseEvent);
        if (!result.isEmpty()) {
            return result;
        }
    }
    return QString();
}
} // namespace QtAda::core
