#pragma once

#include <QString>

namespace QtAda {
Q_GLOBAL_STATIC(bool, doHighlight);

// General data
// static constexpr char QTADA_NAME[] = "QtAda";

// Templates for console messages
inline constexpr char MSG_ERR[] = "[QtAda ERROR] ";
inline constexpr char MSG_WRN[] = "[QtAda  WARN] ";
inline constexpr char MSG_INF[] = "[QtAda  INFO] ";
inline constexpr char MSG_OK[] = "[QtAda    OK] ";

// Project data
inline constexpr char PROJECT_FORMAT[] = "qtada";
inline constexpr char PROJECT_TMP_FORMAT[] = "qtada_tmp";
inline constexpr char PROJECT_AUT_GROUP[] = "Applications";

// Headers for MessageBoxes
inline constexpr char HEADER_PROJECT_ERR[] = QT_TR_NOOP("QtAda | Project Error");
inline constexpr char HEADER_PROJECT_WRN[] = QT_TR_NOOP("QtAda | Project Warning");
}
