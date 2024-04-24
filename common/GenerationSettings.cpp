#include "GenerationSettings.hpp"

#include <QJsonObject>
#include <QJsonDocument>

namespace QtAda::common {
QByteArray GenerationSettings::toJson() const noexcept
{
    QJsonObject obj;
    obj["scriptPath"] = this->scriptPath;
    obj["scriptWriteMode"] = static_cast<int>(this->scriptWriteMode);
    obj["appendLineIndex"] = this->appendLineIndex;
    obj["indentWidth"] = this->indentWidth;
    obj["textIndexBehavior"] = static_cast<int>(this->textIndexBehavior);
    obj["duplicateMouseEvent"] = this->duplicateMouseEvent;
    obj["needToGenerateCycle"] = this->needToGenerateCycle;
    obj["cycleMinimumCount"] = this->cycleMinimumCount;
    obj["closeWindowsOnExit"] = this->closeWindowsOnExit;
    obj["blockCommentMinimumCount"] = this->blockCommentMinimumCount;

    const auto document = QJsonDocument(obj);
    return document.toJson(QJsonDocument::Indented);
}

const GenerationSettings GenerationSettings::fromJson(const QByteArray &data) noexcept
{
    QJsonDocument readDoc = QJsonDocument::fromJson(data);
    QJsonObject obj = readDoc.object();

    GenerationSettings settings;
    settings.scriptPath = obj["scriptPath"].toString();
    settings.scriptWriteMode = static_cast<ScriptWriteMode>(obj["scriptWriteMode"].toInt());
    settings.appendLineIndex = obj["appendLineIndex"].toInt();
    settings.indentWidth = obj["indentWidth"].toInt();
    settings.textIndexBehavior = static_cast<TextIndexBehavior>(obj["textIndexBehavior"].toInt());
    settings.duplicateMouseEvent = obj["duplicateMouseEvent"].toBool();
    settings.needToGenerateCycle = obj["needToGenerateCycle"].toBool();
    settings.cycleMinimumCount = obj["cycleMinimumCount"].toInt();
    settings.closeWindowsOnExit = obj["closeWindowsOnExit"].toBool();
    settings.blockCommentMinimumCount = obj["blockCommentMinimumCount"].toInt();
    return settings;
}
} // namespace QtAda::common
