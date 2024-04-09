#pragma once

#include <qnamespace.h>
#include "ProcessedObjects.hpp"

QT_BEGIN_NAMESPACE
class QWidget;
class QMouseEvent;
class QItemSelectionModel;
class QLatin1String;
QT_END_NAMESPACE

namespace QtAda::core::utils {
QString objectPath(const QWidget *widget) noexcept;
QString widgetIdInView(const QWidget *widget, const int index,
                       const WidgetClass widgetClass) noexcept;
QString selectedCellsData(const QItemSelectionModel *model) noexcept;

bool mouseEventCanBeFiltered(const QWidget *widget, const QMouseEvent *event,
                             bool shouldBePressEvent = false) noexcept;

std::pair<const QWidget *, size_t> searchSpecificWidgetWithIteration(
    const QWidget *widget, const std::pair<QLatin1String, size_t> &classDesignation) noexcept;
const QWidget *
searchSpecificWidget(const QWidget *widget,
                     const std::pair<QLatin1String, size_t> &classDesignation) noexcept;
} // namespace QtAda::core::utils
