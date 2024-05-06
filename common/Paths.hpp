#pragma once

#include <QDir>

namespace QtAda::paths {
static const auto QTADA_CONFIG = QStringLiteral("%1/.config/qtada.conf").arg(QDir::homePath());
static constexpr char PROJECT_SUFFIX[] = "qtada";
static constexpr char PROJECT_TMP_SUFFIX[] = "qtada_tmp";

static constexpr char CONFIG_RECENT_PROJECTS[] = "recentProjects";

static const auto QTADA_HEADER = QStringLiteral("QtAda");
static const auto QTADA_ERROR_HEADER = QStringLiteral("%1 | Error").arg(QTADA_HEADER);
static const auto QTADA_WARNING_HEADER = QStringLiteral("%1 | Warning").arg(QTADA_HEADER);
static const auto QTADA_PROJECT_ERROR_HEADER
    = QStringLiteral("%1 | Project Error").arg(QTADA_HEADER);
static const auto QTADA_INIT_PROJECT_HEADER = QStringLiteral("%1 | Init Project").arg(QTADA_HEADER);
static const auto QTADA_OPEN_PROJECT_HEADER = QStringLiteral("%1 | Open Project").arg(QTADA_HEADER);
static const auto QTADA_NEW_PROJECT_HEADER = QStringLiteral("%1 | New Project").arg(QTADA_HEADER);
static const auto QTADA_NEW_SCRIPT_HEADER = QStringLiteral("%1 | New Script").arg(QTADA_HEADER);
static const auto QTADA_NEW_SOURCE_HEADER = QStringLiteral("%1 | New Source").arg(QTADA_HEADER);
static const auto QTADA_ADD_SCRIPT_HEADER = QStringLiteral("%1 | Add Script").arg(QTADA_HEADER);
static const auto QTADA_ADD_SOURCE_HEADER = QStringLiteral("%1 | Add Source").arg(QTADA_HEADER);
static const auto QTADA_SELECT_EXE_HEADER
    = QStringLiteral("%1 | Select Executable").arg(QTADA_HEADER);
static const auto QTADA_UNSAVED_CHANGES_HEADER
    = QStringLiteral("%1 | Unsaved Changes").arg(QTADA_HEADER);
static const auto QTADA_OVERWRITE_SCRIPT_HEADER
    = QStringLiteral("%1 | Overwrite confirm").arg(QTADA_HEADER);
static const auto QTADA_RECORD_CONTROLLER_HEADER
    = QStringLiteral("%1 | Record Controller").arg(QTADA_HEADER);

static constexpr char PROJECT_APP_PATH[] = "appPath";
static constexpr char PROJECT_SCRIPTS[] = "scripts";
static constexpr char PROJECT_SOURCES[] = "sources";
static constexpr char PROJECT_GUI_GROUP[] = "LastGuiParams";
static constexpr char PROJECT_CONTENT_SIZES[] = "contentSizes";
static constexpr char PROJECT_MAIN_SIZES[] = "mainSizes";
static constexpr char PROJECT_TOOL_BAR_POSITION[] = "toolBarPosition";
static constexpr char PROJECT_LINE_WRAP_MODE[] = "lineWrapMode";
static constexpr char PROJECT_RECORD_GROUP[] = "SettingsForRecord";
static constexpr char PROJECT_RUN_GROUP[] = "SettingsForRun";
static constexpr char PROJECT_LAUNCH_GROUP[] = "SettingsForLaunch";
static constexpr char PROJECT_WORKING_DIR[] = "workingDirectory";
static constexpr char PROJECT_TIMEOUT[] = "timeout";

static constexpr char REMOTE_OBJECT_PATH[] = "local:QTADA_REMOTE_OBJECT";
} // namespace QtAda::paths
