#pragma once

#include <QtGlobal>
#include <functional>
#include <optional>

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QString;
class QWidget;
QT_END_NAMESPACE

namespace QtAda::core {
QString qMouseEventFilter(const QString &path, QWidget *widget, QMouseEvent *event);

// Press filters:
QString qComboBoxFilter(QWidget *widget, QMouseEvent *event, bool);
QString qButtonFilter(QWidget *widget, QMouseEvent *event, bool);
QString qCheckBoxFilter(QWidget *widget, QMouseEvent *event, bool);

// Delayed filters:
QString qSpinBoxFilter(QWidget *widget, QMouseEvent *event, bool isDelayed);
} // namespace QtAda::core
