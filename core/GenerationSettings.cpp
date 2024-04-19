#include "GenerationSettings.hpp"

namespace QtAda::core {
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
                                         "cycleMinimumCount=3;");

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
            bool isOk = false;
            const auto intValue = value.toInt(&isOk);
            assert(isOk == true && intValue >= 0
                   && intValue < static_cast<int>(ScriptWriteMode::None));
            settings_.scriptWriteMode = static_cast<ScriptWriteMode>(intValue);
        }
        else if (key == QLatin1String("appendLineIndex")) {
            bool isOk = false;
            const auto intValue = value.toInt(&isOk);
            assert(isOk == true && intValue >= 0);
            settings_.appendLineIndex = intValue;
        }
        else if (key == QLatin1String("indentWidth")) {
            bool isOk = false;
            const auto intValue = value.toInt(&isOk);
            assert(isOk == true && intValue >= 0);
            settings_.indentWidth = intValue;
        }
        else if (key == QLatin1String("textIndexBehavior")) {
            bool isOk = false;
            const auto intValue = value.toInt(&isOk);
            assert(isOk == true && intValue >= 0
                   && intValue < static_cast<int>(TextIndexBehavior::None));
            settings_.textIndexBehavior = static_cast<TextIndexBehavior>(intValue);
        }
        else if (key == QLatin1String("duplicateMouseEvent")) {
            bool isOk = false;
            const auto intValue = value.toInt(&isOk);
            assert(isOk && (intValue == 0 || intValue == 1));
            settings_.duplicateMouseEvent = intValue != 0;
        }
        else if (key == QLatin1String("needToGenerateCycle")) {
            bool isOk = false;
            const auto intValue = value.toInt(&isOk);
            assert(isOk && (intValue == 0 || intValue == 1));
            settings_.needToGenerateCycle = intValue != 0;
        }
        else if (key == QLatin1String("cycleMinimumCount")) {
            bool isOk = false;
            const auto intValue = value.toInt(&isOk);
            assert(isOk == true && intValue >= CYCLE_MINIMUM_COUNT);
            settings_.cycleMinimumCount = intValue;
        }
    }
}
} // namespace QtAda::core
