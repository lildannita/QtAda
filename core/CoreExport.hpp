#include <qglobal.h>

#ifdef CORE_BUILD
#define CORE_EXPORT Q_DECL_EXPORT
#else
#define CORE_EXPORT Q_DECL_IMPORT
#endif
