#include "Hooks.hpp"

//#include "ProbeInitializer.hpp"

#include <QObject>
#include <QCoreApplication>
#include <private/qhooks_p.h>

static void (*next_startupHook)() = nullptr;
static void (*next_objectAddedHook)(QObject *) = nullptr;
static void (*next_objectRemovedHook)(QObject *) = nullptr;

extern Q_DECL_EXPORT void startupHook()
{
    // Probe::startupHook
//    new probe::ProbeInitializer();

    if (next_startupHook != nullptr) {
        next_startupHook();
    }
}

extern Q_DECL_EXPORT void objectAddedHook(QObject *obj)
{
    // Hooks::added
    if (next_objectAddedHook != nullptr) {
        next_objectAddedHook(obj);
    }
}

extern Q_DECL_EXPORT void objectRemovedHook(QObject *obj)
{
    // Hooks::removed
    if (next_objectRemovedHook != nullptr) {
        next_objectRemovedHook(obj);
    }
}

static void internalHooksInstall()
{
    Q_ASSERT(qtHookData[QHooks::HookDataVersion] >= 1);
    Q_ASSERT(qtHookData[QHooks::HookDataSize] >= 6);

    next_startupHook
        = reinterpret_cast<QHooks::StartupCallback>(qtHookData[QHooks::Startup]);
    next_objectAddedHook
        = reinterpret_cast<QHooks::AddQObjectCallback>(qtHookData[QHooks::AddQObject]);
    next_objectRemovedHook = reinterpret_cast<QHooks::RemoveQObjectCallback>(
        qtHookData[QHooks::RemoveQObject]);

    qtHookData[QHooks::Startup] = reinterpret_cast<quintptr>(&startupHook);
    qtHookData[QHooks::AddQObject] = reinterpret_cast<quintptr>(&objectAddedHook);
    qtHookData[QHooks::RemoveQObject] = reinterpret_cast<quintptr>(&objectRemovedHook);
}

static bool hooksInstalled()
{
    return qtHookData[QHooks::Startup] == reinterpret_cast<quintptr>(&startupHook);
}

static void installHooks()
{
    if (hooksInstalled()) {
        return;
    }
    internalHooksInstall();
}
Q_COREAPP_STARTUP_FUNCTION(installHooks)
