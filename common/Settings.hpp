#pragma once

#include <QString>
#include <optional>
#include <vector>

#include "Common.hpp"

namespace QtAda {
enum class LaunchType {
    None = 0,
    Record = 1,
    Run = 2,
};

enum class TextIndexBehavior {
    OnlyIndex = 0,
    OnlyText = 1,
    TextIndex = 2,
};

enum class ScriptWriteMode {
    NewScript = 0,
    UpdateScript = 1,
};

struct RecordSettings final {
    QString scriptPath = QString();

    ScriptWriteMode scriptWriteMode = ScriptWriteMode::NewScript;
    int appendLineIndex = -1;

    QString executeArgs = QString();

    int indentWidth = DEFAULT_INDENT_WIDTH;
    int blockCommentMinimumCount = 0;
    bool duplicateMouseEvent = false;
    TextIndexBehavior textIndexBehavior = TextIndexBehavior::OnlyIndex;

    bool needToGenerateCycle = false;
    int cycleMinimumCount = MINIMUM_CYCLE_COUNT;

    std::optional<std::vector<QString>> findErrors() const noexcept;
    bool isValid() const noexcept
    {
        return !findErrors().has_value();
    }
    const QByteArray toJson(bool forGui = false) const noexcept;
    static const RecordSettings fromJson(const QByteArray &data, bool forGui = false) noexcept;
};

struct RunSettings final {
    QString scriptPath = QString();

    QString executeArgs = QString();

    int attempsNumber = DEFAULT_ATTEMPS_NUMBER;
    int retryInterval = DEFAULT_RETRY_INTERVAL;

    std::optional<std::vector<QString>> findErrors() const noexcept;
    bool isValid() const noexcept
    {
        return !findErrors().has_value();
    }
    const QByteArray toJson(bool forGui = false) const noexcept;
    static const RunSettings fromJson(const QByteArray &data, bool forGui = false) noexcept;
};
} // namespace QtAda
