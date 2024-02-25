#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QObject;
QT_END_NAMESPACE

extern "C" {
extern Q_DECL_EXPORT void startupHook();
extern Q_DECL_EXPORT void objectAddedHook(QObject *obj);
extern Q_DECL_EXPORT void objectRemovedHook(QObject *obj);
}

namespace hooks {
void installHooks();
}
