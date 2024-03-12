#pragma once

#include <QEvent>
#include <QtGlobal>
#include <qnamespace.h>

QT_BEGIN_NAMESPACE
class QObject;
class QString;
class QWidget;
class QModelIndex;
QT_END_NAMESPACE

namespace QtAda::core::utils {
enum WidgetClass {
    ComboBox,
    SpinBox,
    CheckBox,
    Button,
};

QString objectPath(const QObject *obj) noexcept;
QString mouseButtonToString(Qt::MouseButton mouseButton) noexcept;

bool mouseEventCanBeFiltered(QWidget *widget, QEvent *event) noexcept;

QWidget *searchSpecificWidget(QWidget *widget, WidgetClass widgetClass,
                              size_t limit = size_t(-1)) noexcept;
std::pair<QWidget *, size_t> searchSpecificWidgetWithIteration(QWidget *widget,
                                                               WidgetClass widgetClass,
                                                               size_t limit = size_t(-1)) noexcept;
QString itemIdInWidgetView(QWidget *widget, QModelIndex index, WidgetClass widgetClass) noexcept;
} // namespace QtAda::core::utils
