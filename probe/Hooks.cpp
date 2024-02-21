#include "Hooks.hpp"

extern "C" __attribute__((visibility("default"))) int rand() { return 42; }
