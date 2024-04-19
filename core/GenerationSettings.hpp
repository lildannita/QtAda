#pragma once

#include <QFileInfo>
#include <QDir>

namespace QtAda::core {
static const int GENERATION_SETTINGS_COUNT = 3;

enum class TextIndexBehavior {
    OnlyIndex = 0,
    OnlyText = 1,
    TextIndex = 2,
    None = 3,
};

struct GenerationSettings final {
    TextIndexBehavior textIndexBehavior = TextIndexBehavior::None;
    bool duplicateMouseEvent = false;
    QFileInfo scriptFileInfo = QString();

    bool isInit() const noexcept
    {
        return textIndexBehavior != TextIndexBehavior::None && scriptFileInfo.dir().isReadable()
               && scriptFileInfo.dir().isAbsolute();
    }
};
} // namespace QtAda::core
