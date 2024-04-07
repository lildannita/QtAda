#pragma once

#include <qnamespace.h>
#include <QString>
#include <QWidget>
#include <QEvent>
#include <optional>

#include "ProcessedObjects.hpp"

QT_BEGIN_NAMESPACE
class QObject;
class QModelIndex;
class QLatin1String;
class QMenu;
class QAction;
class QItemSelectionModel;
QT_END_NAMESPACE

namespace QtAda::core::utils {
QString escapeText(const QString &text) noexcept;

QString objectPath(const QObject *obj) noexcept;
QString mouseButtonToString(const Qt::MouseButton mouseButton) noexcept;

bool mouseEventCanBeFiltered(const QWidget *widget, const QMouseEvent *event,
                             bool shouldBePressEvent = false) noexcept;

QString widgetIdInView(const QWidget *widget, const int index,
                       const WidgetClass widgetClass) noexcept;
QString selectedCellsData(const QItemSelectionModel *model) noexcept;

std::pair<const QWidget *, size_t> searchSpecificWidgetWithIteration(
    const QWidget *widget, const std::pair<QLatin1String, size_t> &classDesignation) noexcept;
const QWidget *
searchSpecificWidget(const QWidget *widget,
                     const std::pair<QLatin1String, size_t> &classDesignation) noexcept;

template <typename T> inline QString setValueStatement(const QWidget *widget, T value) noexcept
{
    static_assert(std::is_arithmetic<T>::value, "Type T must be a digit");
    return QStringLiteral("setValue('%1', %2);").arg(objectPath(widget)).arg(value);
}

inline QString setValueStatement(const QWidget *widget, const QString &value) noexcept
{
    return QStringLiteral("setValue('%1', '%2');").arg(objectPath(widget), value);
}

inline QString changeValueStatement(const QWidget *widget, const QString &type) noexcept
{
    return QStringLiteral("changeValue('%1', '%2');").arg(objectPath(widget), type);
}
} // namespace QtAda::core::utils
