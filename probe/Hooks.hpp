#pragma once

#include <QtGlobal>

class QObject;

extern "C" {
extern Q_DECL_EXPORT void startupHook();
extern Q_DECL_EXPORT void objectAddedHook(QObject *obj);
extern Q_DECL_EXPORT void objectRemovedHook(QObject *obj);
}

namespace hooks {
void installHooks();
}
