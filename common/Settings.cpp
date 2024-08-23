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
    const auto opened = file.open(QIODevice::ReadOnly | QIODevice::Text);
    assert(opened == true);

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

    auto dir = fileInfo.dir();
    if (!dir.isReadable()) {
        return false;
    }

    QFile tmp(QStringLiteral("%1/tmp.%2").arg(fileInfo.path(), paths::PROJECT_TMP_SUFFIX));
    if (tmp.open(QIODevice::WriteOnly)) {
        tmp.close();
        tmp.remove();
        return true;
    }
    return false;
}

static std::vector<QString>
findFileErrors(const QString &scriptPath, const QString &confPath,
               std::optional<const RecordSettings *> recordSettings) noexcept
{
    std::vector<QString> errors;
    bool isForRecord = recordSettings.has_value();

    bool scriptPathIsEmpty = scriptPath.isEmpty();
    bool confPathIsEmpty = confPath.isEmpty();
    if (scriptPathIsEmpty || confPathIsEmpty) {
        if (scriptPathIsEmpty) {
            errors.push_back(QStringLiteral("Script path is not specified."));
        }
        if (confPathIsEmpty) {
            errors.push_back(QStringLiteral("Configuration path is not specified."));
        }
        assert(errors.size() > 0);
    }
    else {
        QFileInfo script(scriptPath);

        bool isForUpdate
            = isForRecord && (*recordSettings)->scriptWriteMode == ScriptWriteMode::UpdateScript;
        bool isJs = script.suffix() == QStringLiteral("js");
        bool canWrite = isForRecord ? fileCanBeWritten(script) : true;

        if (isForUpdate || !isForRecord) {
            if (!script.exists()) {
                errors.push_back(
                    QStringLiteral("The script does not exist at '%1'.").arg(scriptPath));
            }
            else if (!script.isFile()) {
                errors.push_back(
                    QStringLiteral("The script at '%1' does not point to a file.").arg(scriptPath));
            }
            else if (!script.isReadable()) {
                errors.push_back(
                    QStringLiteral("The script at '%1' cannot be read.").arg(scriptPath));
            }
            else if (isForUpdate && canWrite && isJs) {
                const auto linesCount = countScriptLines(script.filePath());
                if ((*recordSettings)->appendLineIndex < 1
                    || (*recordSettings)->appendLineIndex > linesCount) {
                    errors.push_back(
                        QStringLiteral("The line index (= %1) for insertion is invalid.")
                            .arg((*recordSettings)->appendLineIndex));
                }
            }
        }

        if (!canWrite) {
            errors.push_back(
                QStringLiteral("The script at '%1' cannot be written.").arg(scriptPath));
        }
        else if (!isJs) {
            errors.push_back(
                QStringLiteral("The script at '%1' must be a JavaScript file with a .js extension.")
                    .arg(scriptPath));
        }

        QFileInfo conf(confPath);
        if (conf.suffix() != QStringLiteral("json")) {
            errors.push_back(
                QStringLiteral(
                    "The configuration at '%1' must be a JSON file with a .json extension.")
                    .arg(confPath));
        }
        else if (conf.exists()) {
            if (!conf.isFile()) {
                errors.push_back(
                    QStringLiteral("The configuration at '%1' does not point to a file.")
                        .arg(confPath));
            }
            else if (isForRecord && !fileCanBeWritten(conf)) {
                errors.push_back(
                    QStringLiteral("The configuration at '%1' cannot be written.").arg(confPath));
            }
            else if (!isForRecord && !conf.isReadable()) {
                errors.push_back(
                    QStringLiteral("The configuration at '%1' cannot be read.").arg(confPath));
            }
            else if (conf.exists()) {
                QFile confFile(conf.absoluteFilePath());
                bool opened = confFile.open(QIODevice::ReadOnly | QIODevice::Text);
                assert(opened == true);

                const auto jsonData = confFile.readAll();
                confFile.close();

                if (!jsonData.trimmed().isEmpty()) {
                    QJsonParseError jsonError;
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &jsonError);
                    if (jsonError.error != QJsonParseError::NoError) {
                        errors.push_back(
                            QStringLiteral("The configuration at '%1' JSON parse error: %2.")
                                .arg(confPath, jsonError.errorString()));
                    }
                    else if (!jsonDoc.isArray()) {
                        errors.push_back(
                            QStringLiteral("The configuration at '%1' is not a JSON-array.")
                                .arg(confPath));
                    }
                }
            }
        }
    }

    return errors;
}

std::optional<std::vector<QString>> RecordSettings::findErrors() const noexcept
{
    auto errors = findFileErrors(scriptPath, confPath, this);
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

    obj["confPath"] = this->confPath;

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

    settings.confPath = obj["confPath"].toString();

    return settings;
}

std::optional<std::vector<QString>> RunSettings::findErrors() const noexcept
{
    const auto errors = findFileErrors(scriptPath, confPath, std::nullopt);
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
    obj["showElapsed"] = this->showElapsed;

    obj["confPath"] = this->confPath;

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
    settings.showElapsed = obj["showElapsed"].toBool();

    settings.confPath = obj["confPath"].toString();

    return settings;
}

} // namespace QtAda
