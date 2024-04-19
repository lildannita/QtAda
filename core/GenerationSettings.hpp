#pragma once

#include <QFileInfo>
#include <QDir>
#include <variant>

namespace QtAda::core {
static const int CYCLE_MINIMUM_COUNT = 3;

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

struct GenerationSettingsWriter final {
    QString scriptPath = QString();
    ScriptWriteMode scriptWriteMode = ScriptWriteMode::None;

    int appendLineIndex = -1;
    int indentWidth = -1;

    TextIndexBehavior textIndexBehavior = TextIndexBehavior::None;
    bool duplicateMouseEvent = false;

    bool needToGenerateCycle = false;
    int cycleMinimumCount = -1;

    /*
     * Для надежности используем этот вектор, который позволяет точно задать последовательность
     * параметров при записи в переменную окружения, а также при чтении из нее (что дает нам
     * дополнительную надежность и мы будем уверены, что точно все проинициализировали).
     * Специально оформлено в виде статической функции, а не константы, так как обращаемся мы
     * к этому вектору два раза - при записи (чтении) в (из) переменную окружения при запуске
     * тестируемого приложения.
     */
    static std::vector<QLatin1String> parametersList() noexcept
    {
        return {
            QLatin1String("scriptPath"),          QLatin1String("scriptWriteMode"),
            QLatin1String("appendLineIndex"),     QLatin1String("indentWidth"),
            QLatin1String("textIndexBehavior"),   QLatin1String("duplicateMouseEvent"),
            QLatin1String("needToGenerateCycle"), QLatin1String("cycleMinimumCount"),
        };
    }

    std::vector<std::variant<QString, int, bool>> valuesList() const noexcept
    {
        return { scriptPath,
                 static_cast<int>(scriptWriteMode),
                 appendLineIndex,
                 indentWidth,
                 static_cast<int>(textIndexBehavior),
                 duplicateMouseEvent,
                 needToGenerateCycle,
                 cycleMinimumCount };
    }

    QString generateStringSettings() const noexcept;
};

class GenerationSettings final {
public:
    GenerationSettings() noexcept;
    GenerationSettings(const GenerationSettings &other)
        : settings_(other.settings_)
    {
    }
    GenerationSettings &operator=(const GenerationSettings &other)
    {
        if (this != &other) {
            settings_ = other.settings_;
        }
        return *this;
    }

    QString scriptPath() const noexcept
    {
        return settings_.scriptPath;
    }
    ScriptWriteMode scriptWriteMode() const noexcept
    {
        return settings_.scriptWriteMode;
    }
    int appendLineIndex() const noexcept
    {
        return settings_.appendLineIndex;
    }
    int indentWidth() const noexcept
    {
        return settings_.indentWidth;
    }
    TextIndexBehavior textIndexBehavior() const noexcept
    {
        return settings_.textIndexBehavior;
    }
    bool duplicateMouseEvent() const noexcept
    {
        return settings_.duplicateMouseEvent;
    }
    bool needToGenerateCycle() const noexcept
    {
        return settings_.needToGenerateCycle;
    }
    int cycleMinimumCount() const noexcept
    {
        return settings_.cycleMinimumCount;
    }

private:
    GenerationSettingsWriter settings_;
};
} // namespace QtAda::core
