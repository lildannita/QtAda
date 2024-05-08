#include "Settings.hpp"

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>

#include "Paths.hpp"

namespace QtAda {
static int countScriptLines(const QString &filePath)
{
    QFile file(filePath);
    assert(file.open(QIODevice::ReadOnly | QIODevice::Text));

    QTextStream in(&file);
    int lineCount = 0;

    while (!in.atEnd()) {
        in.readLine();
        lineCount++;
    }

    file.close();
    return lineCount;
}

static bool fileCanBeWritten(const QFileInfo &fileInfo) noexcept
{
    if (fileInfo.exists()) {
        return fileInfo.isWritable() && fileInfo.isReadable();
    }

    auto scriptDir = fileInfo.dir();
    if (!scriptDir.isReadable()) {
        return false;
    }

    QFile tmp(QStringLiteral("%1/tmp.%2").arg(fileInfo.path()).arg(paths::PROJECT_TMP_SUFFIX));
    if (tmp.open(QIODevice::WriteOnly)) {
        tmp.close();
        tmp.remove();
        return true;
    }
    return false;
}

std::optional<std::vector<QString>> RecordSettings::findErrors() const noexcept
{
    std::vector<QString> errors;

    if (scriptPath.isEmpty()) {
        errors.push_back(QStringLiteral("Script path is not specified."));
    }
    else {
        QFileInfo script(scriptPath);

        bool isJs = script.suffix() == QStringLiteral("js");
        bool isScript = script.exists() && script.isFile() && isJs;
        bool canWrite = fileCanBeWritten(script);

        if (scriptWriteMode == ScriptWriteMode::NewScript) {
            if (!canWrite) {
                errors.push_back(
                    QStringLiteral("The script at the specified path cannot be written."));
            }
            else if (!isJs) {
                errors.push_back(
                    QStringLiteral("The script must be a JavaScript file with a .js extension."));
            }
        }
        else if (scriptWriteMode == ScriptWriteMode::UpdateScript) {
            if (!script.exists()) {
                errors.push_back(
                    QStringLiteral("The script does not exist at the specified path."));
            }
            else if (!script.isFile()) {
                errors.push_back(QStringLiteral("The specified path does not point to a file."));
            }
            else if (!canWrite) {
                errors.push_back(
                    QStringLiteral("The script at the specified path cannot be written."));
            }
            else if (!isJs) {
                errors.push_back(
                    QStringLiteral("The script must be a JavaScript file with a .js extension."));
            }
            else {
                const auto linesCount = countScriptLines(script.filePath());
                if (appendLineIndex < 1 || appendLineIndex > linesCount) {
                    errors.push_back(QStringLiteral("The line index for insertion is invalid."));
                }
            }
        }
    }

    if (needToGenerateCycle && cycleMinimumCount < MINIMUM_CYCLE_COUNT) {
        errors.push_back(QStringLiteral("The minimum number of lines for loop generation is less "
                                        "than the required minimum of %1.")
                             .arg(MINIMUM_CYCLE_COUNT));
    }

    if (indentWidth < 0) {
        errors.push_back(QStringLiteral("Invalid indentation value."));
    }

    return errors.empty() ? std::nullopt : std::make_optional(errors);
}

const QByteArray RecordSettings::toJson(bool forGui) const noexcept
{
    QJsonObject obj;
    if (forGui) {
        obj["executeArgs"] = this->executeArgs;
    }
    else {
        obj["scriptPath"] = this->scriptPath;
        obj["scriptWriteMode"] = static_cast<int>(this->scriptWriteMode);
    }
    obj["appendLineIndex"] = this->appendLineIndex;
    obj["indentWidth"] = this->indentWidth;
    obj["blockCommentMinimumCount"] = this->blockCommentMinimumCount;
    obj["duplicateMouseEvent"] = this->duplicateMouseEvent;
    obj["textIndexBehavior"] = static_cast<int>(this->textIndexBehavior);
    obj["needToGenerateCycle"] = this->needToGenerateCycle;
    obj["cycleMinimumCount"] = this->cycleMinimumCount;
    const auto document = QJsonDocument(obj);
    return document.toJson(QJsonDocument::Indented);
}

const RecordSettings RecordSettings::fromJson(const QByteArray &data, bool forGui) noexcept
{
    QJsonDocument readDoc = QJsonDocument::fromJson(data);
    QJsonObject obj = readDoc.object();
    RecordSettings settings;
    if (forGui) {
        settings.executeArgs = obj["executeArgs"].toString();
    }
    else {
        settings.scriptPath = obj["scriptPath"].toString();
        settings.scriptWriteMode = static_cast<ScriptWriteMode>(obj["scriptWriteMode"].toInt());
    }
    settings.appendLineIndex = obj["appendLineIndex"].toInt();
    settings.indentWidth = obj["indentWidth"].toInt();
    settings.blockCommentMinimumCount = obj["blockCommentMinimumCount"].toInt();
    settings.duplicateMouseEvent = obj["duplicateMouseEvent"].toBool();
    settings.textIndexBehavior = static_cast<TextIndexBehavior>(obj["textIndexBehavior"].toInt());
    settings.needToGenerateCycle = obj["needToGenerateCycle"].toBool();
    settings.cycleMinimumCount = obj["cycleMinimumCount"].toInt();
    return settings;
}

std::optional<std::vector<QString>> RunSettings::findErrors() const noexcept
{
    std::vector<QString> errors;

    if (scriptPath.isEmpty()) {
        errors.push_back(QStringLiteral("Script path is not specified."));
    }
    else {
        QFileInfo script(scriptPath);
        if (!script.exists()) {
            errors.push_back(QStringLiteral("The script does not exist at '%1'.").arg(scriptPath));
        }
        else if (!script.isFile()) {
            errors.push_back(QStringLiteral("'%1' does not point to a file.").arg(scriptPath));
        }
        else if (!script.isReadable()) {
            errors.push_back(QStringLiteral("The script at '%1' cannot be read.").arg(scriptPath));
        }
        else if (script.suffix() != QStringLiteral("js")) {
            errors.push_back(
                QStringLiteral("'%1': the script must be a JavaScript file with a .js extension.")
                    .arg(scriptPath));
        }
    }

    if (attempsNumber < MINIMUM_ATTEMPS_NUMBER) {
        errors.push_back(
            QStringLiteral(
                "The number of attemps to retrive object is less than the required minimum of %1.")
                .arg(MINIMUM_ATTEMPS_NUMBER));
    }
    if (retryInterval < MINIMUM_RETRY_INTERVAL) {
        errors.push_back(
            QStringLiteral(
                "The retry interval before next attempt is less than the required minimum of %1.")
                .arg(MINIMUM_RETRY_INTERVAL));
    }
    return errors.empty() ? std::nullopt : std::make_optional(errors);
}

const QByteArray RunSettings::toJson(bool forGui) const noexcept
{
    QJsonObject obj;
    if (forGui) {
        obj["executeArgs"] = this->executeArgs;
    }
    else {
        obj["scriptPath"] = this->scriptPath;
    }
    obj["attempsNumber"] = this->attempsNumber;
    obj["retryInterval"] = this->retryInterval;
    const auto document = QJsonDocument(obj);
    return document.toJson(QJsonDocument::Indented);
}

const RunSettings RunSettings::fromJson(const QByteArray &data, bool forGui) noexcept
{
    QJsonDocument readDoc = QJsonDocument::fromJson(data);
    QJsonObject obj = readDoc.object();
    RunSettings settings;
    if (forGui) {
        settings.executeArgs = obj["executeArgs"].toString();
    }
    else {
        settings.scriptPath = obj["scriptPath"].toString();
    }
    settings.attempsNumber = obj["attempsNumber"].toInt();
    settings.retryInterval = obj["retryInterval"].toInt();
    return settings;
}

} // namespace QtAda
