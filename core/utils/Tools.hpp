#pragma once

#include <dlfcn.h>
#include <vector>
#include <QString>

namespace QtAda::core::tools {
inline bool isReadOnlyData(const void *data)
{
    Dl_info info;
    return dladdr(const_cast<void *>(data), &info) != 0;
}

template <typename T> QString pointerToString(T *ptr) noexcept
{
    return QString("0x%1").arg(reinterpret_cast<quintptr>(ptr), QT_POINTER_SIZE * 2, 16,
                               QChar('0'));
}

std::vector<QString> cutLine(const QString &line) noexcept;
} // namespace QtAda::core::tools
