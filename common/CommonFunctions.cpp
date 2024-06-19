#include "CommonFunctions.hpp"

#include <iostream>
#include <QStringList>

#include "CommonData.hpp"

namespace QtAda {
constexpr auto MSG_LEN = strLength(MSG_ERR);

inline constexpr char HL_INF[] = "\033[1;34m";
inline constexpr char HL_ERR[] = "\033[1;31m";
inline constexpr char HL_WRN[] = "\033[1;33m";
inline constexpr char HL_OK[] = "\033[1;33m";
inline constexpr char HL_CLR[] = "\033[0m";

void printMessage(const QString &msg, MsgType type) noexcept
{
    const char* prefix;
    const char* highlight;

    switch (type) {
    case MsgType::Info:
        prefix = MSG_INF;
        highlight = HL_INF;
        break;
    case MsgType::Error:
        prefix = MSG_ERR;
        highlight = HL_ERR;
        break;
    case MsgType::Warning:
        prefix = MSG_WRN;
        highlight = HL_WRN;
        break;
    case MsgType::Ok:
        prefix = MSG_OK;
        highlight = HL_OK;
        break;
    default:
        Q_UNREACHABLE();
    }

    if (doHighlight) {
        std::cout << highlight << prefix << HL_CLR;
    } else {
        std::cout << prefix;
    }

    const auto lines = msg.split('\n');
    for (const auto &line : lines) {
        std::cout << QStringLiteral("%1").arg(' ', MSG_LEN).arg(line).toStdString() << std::endl;
    }
    std::cout << std::flush;
}
}
