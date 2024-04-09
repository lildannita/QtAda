#pragma once

#include <dlfcn.h>

namespace QtAda::core::tools {
inline bool isReadOnlyData(const void *data)
{
    Dl_info info;
    return dladdr(const_cast<void *>(data), &info) != 0;
}
} // namespace QtAda::core::tools
