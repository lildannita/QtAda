#include "Settings.hpp"

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>

namespace QtAda::common {
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

    QFile tmp(fileInfo.path() + "/tmp.qtada");
    if (tmp.open(QIODevice::WriteOnly)) {
        tmp.close();
        QFile::remove(fileInfo.path());
        return true;
    }
    return false;
}

std::optional<std::vector<QString>> RecordSettings::isValid() const noexcept
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

const QByteArray RecordSettings::toJson() const noexcept
{
    QJsonObject obj;
    obj["scriptPath"] = this->scriptPath;
    obj["inprocessDialog"] = this->inprocessDialog;
    obj["indentWidth"] = this->indentWidth;
    obj["blockCommentMinimumCount"] = this->blockCommentMinimumCount;
    obj["duplicateMouseEvent"] = this->duplicateMouseEvent;
    obj["closeWindowsOnExit"] = this->closeWindowsOnExit;
    obj["textIndexBehavior"] = static_cast<int>(this->textIndexBehavior);
    obj["scriptWriteMode"] = static_cast<int>(this->scriptWriteMode);
    obj["appendLineIndex"] = this->appendLineIndex;
    obj["needToGenerateCycle"] = this->needToGenerateCycle;
    obj["cycleMinimumCount"] = this->cycleMinimumCount;
    const auto document = QJsonDocument(obj);
    return document.toJson(QJsonDocument::Indented);
}

const RecordSettings RecordSettings::fromJson(const QByteArray &data) noexcept
{
    QJsonDocument readDoc = QJsonDocument::fromJson(data);
    QJsonObject obj = readDoc.object();
    RecordSettings settings;
    settings.scriptPath = obj["scriptPath"].toString();
    settings.inprocessDialog = obj["inprocessDialog"].toBool();
    settings.indentWidth = obj["indentWidth"].toInt();
    settings.blockCommentMinimumCount = obj["blockCommentMinimumCount"].toInt();
    settings.duplicateMouseEvent = obj["duplicateMouseEvent"].toBool();
    settings.closeWindowsOnExit = obj["closeWindowsOnExit"].toBool();
    settings.textIndexBehavior = static_cast<TextIndexBehavior>(obj["textIndexBehavior"].toInt());
    settings.scriptWriteMode = static_cast<ScriptWriteMode>(obj["scriptWriteMode"].toInt());
    settings.appendLineIndex = obj["appendLineIndex"].toInt();
    settings.needToGenerateCycle = obj["needToGenerateCycle"].toBool();
    settings.cycleMinimumCount = obj["cycleMinimumCount"].toInt();
    return settings;
}

std::optional<std::vector<QString>> ExecuteSettings::isValid() const noexcept
{
    std::vector<QString> errors;

    if (scriptPath.isEmpty()) {
        errors.push_back(QStringLiteral("Script path is not specified."));
    }
    else {
        QFileInfo script(scriptPath);
        if (!script.exists()) {
            errors.push_back(QStringLiteral("The script does not exist at the specified path."));
        }
        else if (!script.isFile()) {
            errors.push_back(QStringLiteral("The specified path does not point to a file."));
        }
        else if (!script.isReadable()) {
            errors.push_back(QStringLiteral("The script at the specified path cannot be read."));
        }
        else if (script.suffix() != QStringLiteral("js")) {
            errors.push_back(
                QStringLiteral("The script must be a JavaScript file with a .js extension."));
        }
    }
    return errors.empty() ? std::nullopt : std::make_optional(errors);
}

const QByteArray ExecuteSettings::toJson() const noexcept
{
    QJsonObject obj;
    obj["scriptPath"] = this->scriptPath;
    const auto document = QJsonDocument(obj);
    return document.toJson(QJsonDocument::Indented);
}

const ExecuteSettings ExecuteSettings::fromJson(const QByteArray &data) noexcept
{
    QJsonDocument readDoc = QJsonDocument::fromJson(data);
    QJsonObject obj = readDoc.object();
    ExecuteSettings settings;
    settings.scriptPath = obj["scriptPath"].toString();
    return settings;
}

} // namespace QtAda::common
