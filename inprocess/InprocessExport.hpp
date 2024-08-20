#include <qglobal.h>

#ifdef INPROCESS_BUILD
#define INPROCESS_EXPORT Q_DECL_EXPORT
#else
#define INPROCESS_EXPORT Q_DECL_IMPORT
#endif
