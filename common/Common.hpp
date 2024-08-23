#pragma once

#include <QString>
#include <iostream>

namespace QtAda {
Q_GLOBAL_STATIC(bool, noHighlight)

inline void setMsgHighlight(bool isHighlight) noexcept
{
    *noHighlight() = !isHighlight;
}

static constexpr int DEFAULT_SCRIPT_TIMEOUT_MS = 5000;
static constexpr int MAXIMUM_SCRIPT_TIMEOUT_MS = 600000;

static constexpr int DEFAULT_WAITING_TIMER_VALUE = 60;
static constexpr int DEFAULT_INDENT_WIDTH = 4;
static constexpr int MINIMUM_CYCLE_COUNT = 3;

static constexpr char ENV_UNSET_PRELOAD[] = "QTADA_NEED_TO_UNSET_PRELOAD";
static constexpr char ENV_LAUNCH_TYPE[] = "QTADA_LAUNCH_TYPE";
static constexpr char ENV_LAUNCH_SETTINGS[] = "QTADA_LAUNCH_SETTINGS";

static constexpr char RESET_COLOR[] = "\033[0m";
static constexpr char QTADA_ERR_COLOR[] = "\033[37;41m";
static constexpr char QTADA_OUT_COLOR[] = "\033[30;42m";
static constexpr char QTADA_WARN_COLOR[] = "\033[30;43m";
static constexpr char QTADA_SERV_COLOR[] = "\033[30;44m";
static constexpr char QTADA_SCRIPT_ERR_COLOR[] = "\033[1;31m";
static constexpr char QTADA_SCRIPT_WARN_COLOR[] = "\033[1;33m";
static constexpr char QTADA_SCRIPT_OUT_COLOR[] = "\033[1m";
static constexpr char GUI_INACTIVE_LOG_COLOR[] = "#7F7F80";
static constexpr char GUI_SERVICE_LOG_COLOR[] = "#008785";
static constexpr char GUI_ERROR_LOG_COLOR[] = "#FF6666";
static constexpr char GUI_WARN_LOG_COLOR[] = "#D6D84F";
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
 -s, --show-log                                 show application logs during test script execution
 --no-highlight                                 disable highlighting of QtAda messages in the console

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
 --show-elapsed                                 displays elapsed time (in milliseconds) for retrieval and verification (default: disabled)
)")
                           .arg(appPath)
                           .arg(DEFAULT_WAITING_TIMER_VALUE)
                           .arg(DEFAULT_INDENT_WIDTH)
                           .arg(MINIMUM_CYCLE_COUNT);
    std::cout << qPrintable(usage) << std::endl << std::flush;
}

inline void printMessage(const QString &msg, bool needNewLine) noexcept
{
    std::cout << qPrintable(msg);
    if (needNewLine) {
        std::cout << std::endl;
    }
    std::cout << std::flush;
}

inline void printStdMessage(const QString &msg) noexcept
{
    printMessage(msg, false);
}

inline void printQtAdaOutMessage(const QString &msg) noexcept
{
    if (noHighlight) {
        printMessage(msg, true);
        return;
    }

    std::cout << qPrintable(
        QStringLiteral("%1QtAda:%2 %3").arg(QTADA_OUT_COLOR).arg(RESET_COLOR).arg(msg))
              << std::endl
              << std::flush;
}

inline void printQtAdaErrorMessage(const QString &msg) noexcept
{
    if (noHighlight) {
        printMessage(msg, true);
        return;
    }

    std::cout << qPrintable(
        QStringLiteral("%1QtAda:%2 %3").arg(QTADA_ERR_COLOR).arg(RESET_COLOR).arg(msg))
              << std::endl
              << std::flush;
}

inline void printQtAdaWarningMessage(const QString &msg) noexcept
{
    if (noHighlight) {
        printMessage(msg, true);
        return;
    }

    std::cout << qPrintable(
        QStringLiteral("%1QtAda:%2 %3").arg(QTADA_WARN_COLOR).arg(RESET_COLOR).arg(msg))
              << std::endl
              << std::flush;
}

inline void printQtAdaServiceMessage(const QString &msg) noexcept
{
    if (noHighlight) {
        printMessage(msg, true);
        return;
    }

    std::cout << qPrintable(
        QStringLiteral("%1QtAda:%2 %3").arg(QTADA_SERV_COLOR).arg(RESET_COLOR).arg(msg))
              << std::endl
              << std::flush;
}

inline void printScriptErrorMessage(const QString &msg) noexcept
{
    if (noHighlight) {
        printMessage(msg, true);
        return;
    }

    std::cout << QTADA_SCRIPT_ERR_COLOR << qPrintable(msg) << RESET_COLOR << std::endl
              << std::flush;
}

inline void printScriptWarningMessage(const QString &msg) noexcept
{
    if (noHighlight) {
        printMessage(msg, true);
        return;
    }

    std::cout << QTADA_SCRIPT_WARN_COLOR << qPrintable(msg) << RESET_COLOR << std::endl
              << std::flush;
}

inline void printScriptOutMessage(const QString &msg) noexcept
{
    if (noHighlight) {
        printMessage(msg, true);
        return;
    }

    std::cout << QTADA_SCRIPT_OUT_COLOR << qPrintable(msg) << RESET_COLOR << std::endl
              << std::flush;
}
} // namespace QtAda
