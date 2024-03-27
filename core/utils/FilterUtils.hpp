#pragma once

#include <qnamespace.h>

#include "ProcessedObjects.hpp"

QT_BEGIN_NAMESPACE
class QEvent;
class QObject;
class QString;
class QWidget;
class QModelIndex;
class QLatin1String;
QT_END_NAMESPACE

namespace QtAda::core::utils {
QString objectPath(const QObject *obj) noexcept;
QString mouseButtonToString(const Qt::MouseButton mouseButton) noexcept;

bool mouseEventCanBeFiltered(const QWidget *widget, const QEvent *event) noexcept;

QString itemIdInWidgetView(const QWidget *widget, const QModelIndex index,
                           const WidgetClass widgetClass) noexcept;
QString setValueStatement(const QWidget *widget, const QString &value,
                          bool isStringValue = false) noexcept;
QString changeValueStatement(const QWidget *widget, const QString &type) noexcept;

const QWidget *searchSpecificWidget(const QWidget *widget,
                                    const QVector<QLatin1String> &classDesignations,
                                    size_t limit = 1) noexcept;
std::pair<const QWidget *, size_t>
searchSpecificWidgetWithIteration(const QWidget *widget,
                                  const QVector<QLatin1String> &classDesignations,
                                  size_t limit = 1) noexcept;
} // namespace QtAda::core::utils
