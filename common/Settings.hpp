#pragma once

#include <QString>
#include <optional>
#include <vector>

#include "Common.hpp"

namespace QtAda::common {
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

    bool inprocessDialog = true;

    int indentWidth = DEFAULT_INDENT_WIDTH;
    int blockCommentMinimumCount = -1;
    bool duplicateMouseEvent = false;
    bool closeWindowsOnExit = false;
    TextIndexBehavior textIndexBehavior = TextIndexBehavior::OnlyIndex;

    ScriptWriteMode scriptWriteMode = ScriptWriteMode::NewScript;
    int appendLineIndex = -1;

    bool needToGenerateCycle = false;
    int cycleMinimumCount = MINIMUM_CYCLE_COUNT;

    std::optional<std::vector<QString>> isValid() const noexcept;
    const QByteArray toJson() const noexcept;
    static const RecordSettings fromJson(const QByteArray &data) noexcept;
};

struct ExecuteSettings final {
    QString scriptPath = QString();

    std::optional<std::vector<QString>> isValid() const noexcept;
    const QByteArray toJson() const noexcept;
    static const ExecuteSettings fromJson(const QByteArray &data) noexcept;
};
} // namespace QtAda::common
