#pragma once

#include <QFileInfo>
#include <QDir>

namespace QtAda::core {
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
    TextIndexBehavior textIndexBehavior = TextIndexBehavior::None;
    bool duplicateMouseEvent = false;

    QString scriptPath = QString();
    ScriptWriteMode writeMode = ScriptWriteMode::None;
    int appendLineIndex = -1;
    int indentWidth = -1;

    bool isInit() const noexcept
    {
        return textIndexBehavior != TextIndexBehavior::None && indentWidth >= 0
               && !scriptPath.isEmpty()
               && (writeMode == ScriptWriteMode::NewScript
                   || (writeMode == ScriptWriteMode::UpdateScript && appendLineIndex >= 0));
    }
};
} // namespace QtAda::core
