#pragma once

#include <qnamespace.h>
#include <QString>
#include <QObject>

QT_BEGIN_NAMESPACE
class QMouseEvent;
QT_END_NAMESPACE

namespace QtAda::core::utils {
QString escapeText(const QString &text) noexcept;
QString objectPath(const QObject *obj) noexcept;
QString mouseButtonToString(const Qt::MouseButton mouseButton) noexcept;

bool mouseEventCanBeFiltered(const QMouseEvent *event, bool shouldBePressEvent = false) noexcept;

template <typename T> inline QString setValueStatement(const QObject *obj, T value) noexcept
{
    static_assert(std::is_arithmetic<T>::value, "Type T must be a digit");
    return QStringLiteral("setValue('%1', %2);").arg(objectPath(obj)).arg(value);
}

inline QString setValueStatement(const QObject *obj, const QString &value) noexcept
{
    return QStringLiteral("setValue('%1', '%2');").arg(objectPath(obj), value);
}

inline QString changeValueStatement(const QObject *obj, const QString &type) noexcept
{
    return QStringLiteral("changeValue('%1', '%2');").arg(objectPath(obj), type);
}

template <typename T, typename Signal, typename Slot>
QMetaObject::Connection connectIfType(const QObject *widget, const QObject *parent, Signal signal,
                                      Slot slot)
{
    if (auto *castedWidget = qobject_cast<const T *>(widget)) {
        return QObject::connect(castedWidget, signal, parent, slot);
    }
    return {};
}
} // namespace QtAda::core::utils
