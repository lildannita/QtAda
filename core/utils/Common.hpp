#pragma once

#include <QObject>
#include <dlfcn.h>

namespace QtAda::core::utils {
inline bool isReadOnlyData(const void *data)
{
    Dl_info info;
    return dladdr(const_cast<void *>(data), &info) != 0;
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
