#include "Execution.hpp"

#include <dlfcn.h>

namespace QtAda::core::utils {
bool isReadOnlyData(const void *data)
{
    Dl_info info;
    return dladdr(const_cast<void *>(data), &info) != 0;
}
} // namespace QtAda::core::utils
