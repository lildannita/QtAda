#include "ProbeGuard.hpp"

#include <QThreadStorage>

static QThreadStorage<bool> s_lockStorage;

namespace QtAda::core {
ProbeGuard::ProbeGuard() noexcept
    : previosState_(locked())
{
    setLocked(true);
}

ProbeGuard::~ProbeGuard() noexcept
{
    setLocked(previosState_);
}

void ProbeGuard::setLocked(bool isLocked) noexcept
{
    s_lockStorage.localData() = isLocked;
}

bool ProbeGuard::locked() noexcept
{
    if (!s_lockStorage.hasLocalData()) {
        return false;
    }
    return s_lockStorage.localData();
}
} // namespace QtAda::core
