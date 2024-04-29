#pragma once

#include <QDir>

namespace QtAda::paths {
static const auto QTADA_CONFIG = QStringLiteral("%1/.config/qtada.conf").arg(QDir::homePath());
static constexpr char PROJECT_SUFFIX[] = "qtada";
static constexpr char PROJECT_TMP_SUFFIX[] = "qtada-tmp";

static constexpr char CONFIG_RECENT_PROJECTS[] = "recentProjects";

static constexpr char REMOTE_OBJECT_PATH[] = "local:QTADA_REMOTE_OBJECT";
//! TODO: Костыль, см. InprocessController::startInitServer().
static const auto INIT_CONNECTION_SERVER
    = QStringLiteral("%1/QTADA_INIT_CONNECTION_SERVER").arg(QDir::tempPath());
} // namespace QtAda::paths
