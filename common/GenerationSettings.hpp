#pragma once

#include <QString>

namespace QtAda::common {
static const int CYCLE_MINIMUM_COUNT = 3;
static const int BLOCK_COMMENT_MINIMUM_COUNT = 1;

enum class TextIndexBehavior {
    OnlyIndex = 0,
    OnlyText = 1,
    TextIndex = 2,
    None = 3,
};

enum class ScriptWriteMode {
    NewScript = 0,
    UpdateScript = 1,
    None = 2,
};

struct GenerationSettings final {
    QString scriptPath = QString();
    ScriptWriteMode scriptWriteMode = ScriptWriteMode::None;

    int appendLineIndex = -1;
    int indentWidth = -1;

    TextIndexBehavior textIndexBehavior = TextIndexBehavior::None;
    bool duplicateMouseEvent = false;

    bool needToGenerateCycle = false;
    int cycleMinimumCount = -1;

    bool closeWindowsOnExit = false;
    int blockCommentMinimumCount = -1;

    QByteArray toJson() const noexcept;
    static const GenerationSettings fromJson(const QByteArray &data) noexcept;
};
} // namespace QtAda::common
