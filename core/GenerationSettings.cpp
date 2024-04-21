#include "GenerationSettings.hpp"

#include <optional>

namespace QtAda::core {
static bool stringToBool(const QString &s) noexcept
{
    bool isOk = false;
    const auto intValue = s.toInt(&isOk);
    assert(isOk == true);
    assert(intValue == 0 || intValue == 1);
    return intValue != 0;
}

static int stringToInt(const QString &s, std::optional<int> left = std::nullopt,
                       std::optional<int> right = std::nullopt) noexcept
{
    bool isOk = false;
    const auto intValue = s.toInt(&isOk);
    assert(isOk == true);
    assert(!left.has_value() || intValue > *left);
    assert(!right.has_value() || intValue < *right);
    return intValue;
}

QString GenerationSettingsWriter::generateStringSettings() const noexcept
{
    const auto parameters = parametersList();
    const auto values = valuesList();
    assert(parameters.size() == values.size());

    QString result;
    for (size_t i = 0; i < parameters.size(); i++) {
        result += QStringLiteral("%1=%2;")
                      .arg(parameters.at(i))
                      .arg(std::visit(
                          [](auto &&arg) -> QString {
                              using T = std::decay_t<decltype(arg)>;
                              if constexpr (std::is_same_v<T, bool>) {
                                  return arg ? "1" : "0";
                              }
                              else if constexpr (std::is_same_v<T, int>) {
                                  return QString::number(arg);
                              }
                              else if constexpr (std::is_same_v<T, QString>) {
                                  return QString(arg);
                              }
                              Q_UNREACHABLE();
                          },
                          values.at(i)));
    }
    return result;
}

GenerationSettings::GenerationSettings() noexcept
{
    //! TODO: remove, когда будет готов GUI QtAda
    qputenv("QTADA_GENERATION_SETTINGS", "scriptPath=/files/trash/qtada.js;"
                                         "scriptWriteMode=0;"
                                         "appendLineIndex=0;"
                                         "indentWidth=4;"
                                         "textIndexBehavior=2;"
                                         "duplicateMouseEvent=1;"
                                         "needToGenerateCycle=1;"
                                         "cycleMinimumCount=3;"
                                         "closeWindowsOnExit=1;"
                                         "blockCommentMinimumCount=3;");

    const auto envValue = qgetenv("QTADA_GENERATION_SETTINGS");
    qputenv("QTADA_GENERATION_SETTINGS", "");

    const auto settings = QString(envValue).split(';', Qt::SkipEmptyParts);
    const auto parametersList = GenerationSettingsWriter::parametersList();
    assert(settings.size() == parametersList.size());

    for (size_t i = 0; i < parametersList.size(); i++) {
        const auto keyValuePair = settings.at(i).split('=');
        assert(keyValuePair.size() == 2);

        const auto key = keyValuePair[0].trimmed();
        const auto value = keyValuePair[1].trimmed();
        assert(key == parametersList.at(i));

        if (key == QLatin1String("scriptPath")) {
            settings_.scriptPath = value;
        }
        else if (key == QLatin1String("scriptWriteMode")) {
            settings_.scriptWriteMode = static_cast<ScriptWriteMode>(
                stringToInt(value, -1, static_cast<int>(ScriptWriteMode::None)));
        }
        else if (key == QLatin1String("appendLineIndex")) {
            settings_.appendLineIndex = stringToInt(value, -1);
        }
        else if (key == QLatin1String("indentWidth")) {
            settings_.indentWidth = stringToInt(value, -1);
        }
        else if (key == QLatin1String("textIndexBehavior")) {
            settings_.textIndexBehavior = static_cast<TextIndexBehavior>(
                stringToInt(value, -1, static_cast<int>(TextIndexBehavior::None)));
        }
        else if (key == QLatin1String("duplicateMouseEvent")) {
            settings_.duplicateMouseEvent = stringToBool(value);
        }
        else if (key == QLatin1String("needToGenerateCycle")) {
            settings_.needToGenerateCycle = stringToBool(value);
        }
        else if (key == QLatin1String("cycleMinimumCount")) {
            settings_.cycleMinimumCount = stringToInt(value, CYCLE_MINIMUM_COUNT - 1);
        }
        else if (key == QLatin1String("closeWindowsOnExit")) {
            settings_.closeWindowsOnExit = stringToBool(value);
        }
        else if (key == QLatin1String("blockCommentMinimumCount")) {
            settings_.blockCommentMinimumCount
                = stringToInt(value, BLOCK_COMMENT_MINIMUM_COUNT - 1);
        }
    }
}
} // namespace QtAda::core
