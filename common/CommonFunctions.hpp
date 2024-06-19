#pragma once

#include <cstddef>
#include <QString>

namespace QtAda {
constexpr std::size_t strLength(const char* str) {
    return *str ? 1 + strLength(str + 1) : 0;
}

// Message functions
enum class MsgType {
    Info,
    Error,
    Warning,
    Ok,
};
void printMessage(const QString &msg, MsgType type) noexcept;
inline void printInfoMessage(const QString &msg) noexcept
{
    printMessage(msg, MsgType::Info);
}
inline void printErrorMessage(const QString &msg) noexcept
{
    printMessage(msg, MsgType::Error);
}
inline void printWarningMessage(const QString &msg) noexcept
{
    printMessage(msg, MsgType::Warning);
}
inline void printOkMessage(const QString &msg) noexcept
{
    printMessage(msg, MsgType::Ok);
}
}
