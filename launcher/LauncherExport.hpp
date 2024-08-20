#include <qglobal.h>

#ifdef LAUNCHER_BUILD
#define LAUNCHER_EXPORT Q_DECL_EXPORT
#else
#define LAUNCHER_EXPORT Q_DECL_IMPORT
#endif
