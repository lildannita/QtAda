#include <qglobal.h>

#ifdef GUI_BUILD
#define GUI_EXPORT Q_DECL_EXPORT
#else
#define GUI_EXPORT Q_DECL_IMPORT
#endif
