#pragma once

#include <QString>
#include <optional>
#include <vector>

#include "Common.hpp"

namespace QtAda {
enum class LaunchType {
    None = 0,
    Record = 1,
    Execute = 2,
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
    int blockCommentMinimumCount = -1;
    bool duplicateMouseEvent = false;
    bool closeWindowsOnExit = false;
    TextIndexBehavior textIndexBehavior = TextIndexBehavior::OnlyIndex;

    bool needToGenerateCycle = false;
    int cycleMinimumCount = MINIMUM_CYCLE_COUNT;

    std::optional<std::vector<QString>> isValid() const noexcept;
    const QByteArray toJson(bool forGui = false) const noexcept;
    static const RecordSettings fromJson(const QByteArray &data, bool forGui = false) noexcept;
};

struct ExecuteSettings final {
    QString scriptPath = QString();

    QString executeArgs = QString();

    std::optional<std::vector<QString>> isValid() const noexcept;
    const QByteArray toJson(bool forGui = false) const noexcept;
    static const ExecuteSettings fromJson(const QByteArray &data, bool forGui = false) noexcept;
};
} // namespace QtAda
