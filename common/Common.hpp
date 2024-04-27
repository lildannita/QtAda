#pragma once

#include <QString>
#include <iostream>

namespace QtAda {
static constexpr int DEFAULT_WAITING_TIMER_VALUE = 60;
static constexpr int DEFAULT_INDENT_WIDTH = 4;
static constexpr int MINIMUM_CYCLE_COUNT = 3;

static constexpr char ENV_UNSET_PRELOAD[] = "QTADA_NEED_TO_UNSET_PRELOAD";
static constexpr char ENV_LAUNCH_TYPE[] = "QTADA_LAUNCH_TYPE";
static constexpr char ENV_LAUNCH_SETTINGS[] = "QTADA_LAUNCH_SETTINGS";

static constexpr char REMOTE_OBJECT_PATH[] = "local:QTADA_REMOTE_OBJECT";
//! TODO: Костыль, см. InprocessController::startInitServer().
static constexpr char INIT_CONNECTION_SERVER[] = "/tmp/QTADA_INIT_CONNECTION_SERVER";

static constexpr char RESET_COLOR[] = "\033[0m";
static constexpr char APP_ERR_COLOR[] = "\033[33m";
static constexpr char QTADA_ERR_COLOR[] = "\033[37;41m";
static constexpr char QTADA_OUT_COLOR[] = "\033[30;42m";

inline void printUsage(const char *appPath)
{
    const auto usage = QStringLiteral(R"(
Usage: %1 [options] <launch type> <script path> <application> [args]

Launch type:
 -r, --record                           record test script
 -e, --execute                          execute test script

Options:
 -h, --help                             print program help and exit
 -w, --workspace                        set working directory for executable (default: current path)
 -t, --timeout                          application launch timeout in seconds (default: %2 seconds)

(Record options):
 --new-script                           new script will be (over)written to the specified path (default)
 --update-script <line index>           the original script will be appended with new lines from specified line index
                                        (numbering begins with 1)

 --indent-width  <integer value>        sets the indent width in the recorded script (default: %3)
 --block-comment <integer value>        sets the minimum number of lines to create a comment block (default: disabled)
 --duplicate-mouse-event                enables duplication of the mouse action as a comment (default: disabled)
 --close-windows-on-exit                enables auto closing of all windows when recording is completed (default: disabled)

 --generate-cycles                      enables automatic generation of a `for` loop for repetitive actions (default: disabled)
 --cycle-min-count                      sets the minimum number of repetitive lines to generate `for` loop (minimum: %4)

 --only-index                           for actions on model delegates, only its index will be specified (default)
 --only-text                            for actions on model delegates, only its text (if possible) will be specified
 --text-index                           for actions on model delegates, its index and text (if possible) will be specified
)")
                           .arg(appPath)
                           .arg(DEFAULT_WAITING_TIMER_VALUE)
                           .arg(DEFAULT_INDENT_WIDTH)
                           .arg(MINIMUM_CYCLE_COUNT);
    std::cout << qPrintable(usage) << std::endl << std::flush;
}

inline void printStdOutMessage(const QString &msg) noexcept
{
    std::cout << qPrintable(msg) << std::flush;
}

inline void printStdErrMessage(const QString &msg) noexcept
{
    std::cout << APP_ERR_COLOR << qPrintable(msg) << RESET_COLOR << std::flush;
}

inline void printQtAdaOutMessage(const QString &msg) noexcept
{
    std::cout << qPrintable(
        QStringLiteral("%1QtAda:%2 %3").arg(QTADA_OUT_COLOR).arg(RESET_COLOR).arg(msg))
              << std::endl
              << std::flush;
}

inline void printQtAdaErrMessage(const QString &msg) noexcept
{
    std::cout << qPrintable(
        QStringLiteral("%1QtAda:%2 %3").arg(QTADA_ERR_COLOR).arg(RESET_COLOR).arg(msg))
              << std::endl
              << std::flush;
}
} // namespace QtAda
