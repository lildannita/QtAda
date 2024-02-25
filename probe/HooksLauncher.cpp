#include "Hooks.hpp"

namespace hooks {
class HooksLauncher {
public:
    HooksLauncher() { hooks::installHooks(); }
};

static HooksLauncher hooksLauncher;
} // namespace hooks
