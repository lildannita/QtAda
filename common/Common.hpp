#pragma once

#include <QString>
#include <iostream>

namespace QtAda {
static constexpr int DEFAULT_WAITING_TIMER_VALUE = 60;
static constexpr int DEFAULT_INDENT_WIDTH = 4;
static constexpr int MINIMUM_CYCLE_COUNT = 3;
static constexpr int MINIMUM_RETRIEVAL_ATTEMPTS = 1;
static constexpr int DEFAULT_RETRIEVAL_ATTEMPTS = 10;
static constexpr int MINIMUM_RETRIEVAL_INTERVAL = 100;
static constexpr int DEFAULT_RETRIEVAL_INTERVAL = 500;
static constexpr int MINIMUM_VERIFY_ATTEMPTS = 1;
static constexpr int DEFAULT_VERIFY_ATTEMPTS = 5;
static constexpr int MINIMUM_VERIFY_INTERVAL = 10;
static constexpr int DEFAULT_VERIFY_INTERVAL = 100;

static constexpr char ENV_UNSET_PRELOAD[] = "QTADA_NEED_TO_UNSET_PRELOAD";
static constexpr char ENV_LAUNCH_TYPE[] = "QTADA_LAUNCH_TYPE";
static constexpr char ENV_LAUNCH_SETTINGS[] = "QTADA_LAUNCH_SETTINGS";

static constexpr char RESET_COLOR[] = "\033[0m";
static constexpr char QTADA_ERR_COLOR[] = "\033[37;41m";
static constexpr char QTADA_OUT_COLOR[] = "\033[30;42m";
static constexpr char QTADA_SCRIPT_ERR_COLOR[] = "\033[1;31m";
static constexpr char QTADA_SCRIPT_OUT_COLOR[] = "\033[1;97m";
static constexpr char GUI_INACTIVE_LOG_COLOR[] = "#7F7F80";
static constexpr char GUI_SERVICE_LOG_COLOR[] = "#008785";
static constexpr char GUI_ERROR_LOG_COLOR[] = "#FF6666";
static constexpr char GUI_SCRIPT_RESULT_COLOR[] = "#D69444";
static constexpr char GUI_SCRIPT_SERVICE_COLOR[] = "#71B1DA";

inline void printUsage(const char *appPath)
{
    const auto usage = QStringLiteral(R"(
Usage: %1 [options] <launch type> <application> [args]

Launch type:
 -r, --record <script path>                     record test script
 -R, --run <script path> [<script path> ...]    run test script

Options:
 -h, --help                                     print program help and exit
 -w, --workspace                                set working directory for executable (default: current path)
 -t, --timeout                                  application launch timeout in seconds (default: %2 seconds)

(Record options):
 --new-script                                   new script will be (over)written to the specified path (default)
 --update-script <line index>                   the original script will be appended with new lines from specified line index
                                                (numbering begins with 1)

 --indent-width <integer value>                 sets the indent width in the recorded script (default: %3)
 --block-comment <integer value>                sets the minimum number of lines to create a comment block (default: disabled)
 --duplicate-mouse-event                        enables duplication of the mouse action as a comment (default: disabled)

 --generate-cycles                              enables automatic generation of a `for` loop for repetitive actions (default: disabled)
 --cycle-min-count                              sets the minimum number of repetitive lines to generate `for` loop (minimum: %4)

 --only-index                                   for actions on model delegates, only its index will be specified (default)
 --only-text                                    for actions on model delegates, only its text (if possible) will be specified
 --text-index                                   for actions on model delegates, its index and text (if possible) will be specified

(Run options):
 --retrieval-attempts <integer value>           sets the attempts number to retrive object by specified path (minimum: %5, default: %6)
 --retrieval-interval <integer value>           sets the interval (in milliseconds) before next attempt (minimum: %7, default: %8)
 --verify-attempts <integer value>              sets the attempts number to verify the expected value (minimum: %9, default: %10)
 --verify-interval <integer value>              sets the interval (in milliseconds) before next verify attempt (minimum: %11, default: %12)
 --show-elapsed                                 displays elapsed time (in milliseconds) for retrieval and verification (default: disabled)
)")
                           .arg(appPath)
                           .arg(DEFAULT_WAITING_TIMER_VALUE)
                           .arg(DEFAULT_INDENT_WIDTH)
                           .arg(MINIMUM_CYCLE_COUNT)
                           .arg(MINIMUM_RETRIEVAL_ATTEMPTS)
                           .arg(DEFAULT_RETRIEVAL_ATTEMPTS)
                           .arg(MINIMUM_RETRIEVAL_INTERVAL)
                           .arg(DEFAULT_RETRIEVAL_INTERVAL)
                           .arg(MINIMUM_VERIFY_ATTEMPTS)
                           .arg(DEFAULT_VERIFY_ATTEMPTS)
                           .arg(MINIMUM_VERIFY_INTERVAL)
                           .arg(DEFAULT_VERIFY_INTERVAL);
    std::cout << qPrintable(usage) << std::endl << std::flush;
}

inline void printStdMessage(const QString &msg) noexcept
{
    std::cout << qPrintable(msg) << std::flush;
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

inline void printScriptErrorMessage(const QString &msg) noexcept
{
    std::cout << QTADA_SCRIPT_ERR_COLOR << qPrintable(msg) << RESET_COLOR << std::endl
              << std::flush;
}

inline void printScriptOutMessage(const QString &msg) noexcept
{
    std::cout << QTADA_SCRIPT_OUT_COLOR << qPrintable(msg) << RESET_COLOR << std::endl
              << std::flush;
}
} // namespace QtAda
