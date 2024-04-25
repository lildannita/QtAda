#pragma once

#include <QString>
#include <QByteArray>

namespace QtAda::common {
static const auto SERVER_PATH = QString("/tmp/QTADA_PROBE_LISTENER");
static const auto LAUNCHER_DESTROYED = QByteArrayLiteral("LAUNCHER_DESTROYED");
} // namespace QtAda::common
