#include "ScriptWriter.hpp"

#include <QFileInfo>
#include <QDir>

namespace QtAda::inprocess {
ScriptWriter::ScriptWriter(const common::RecordSettings &settings, QObject *parent) noexcept
    : QObject{ parent }
    , recordSettings_{ settings }
    , linesHandler_{ settings.needToGenerateCycle, settings.cycleMinimumCount }
{
    QFileInfo scriptInfo(recordSettings_.scriptPath);
    const auto scriptDir = scriptInfo.absoluteDir();
    assert(scriptDir.exists() && scriptDir.isReadable());
    assert(scriptInfo.suffix() == "js");

    switch (settings.scriptWriteMode) {
    case common::ScriptWriteMode::NewScript: {
        script_.setFileName(recordSettings_.scriptPath);
        bool isOpen = script_.open(QIODevice::WriteOnly | QIODevice::Truncate);
        assert(isOpen == true);
        scriptStream_.setDevice(&script_);
        flushScriptLine("function test() {", 0);
        break;
    }
    case common::ScriptWriteMode::UpdateScript: {
        assert(scriptInfo.exists());

        QFile originalScript(recordSettings_.scriptPath);
        bool isOpen = originalScript.open(QIODevice::ReadOnly | QIODevice::Text);
        assert(isOpen == true);
        QTextStream readScript(&originalScript);
        while (!readScript.atEnd()) {
            savedLines_.push_back(readScript.readLine());
        }
        originalScript.close();

        script_.setFileName(recordSettings_.scriptPath + ".qtada");
        isOpen = script_.open(QIODevice::WriteOnly | QIODevice::Truncate);
        assert(isOpen == true);
        scriptStream_.setDevice(&script_);

        // Так как нумерация при задании параметров начинается с единицы
        auto fromZeroIndex = recordSettings_.appendLineIndex - 1;
        assert(fromZeroIndex >= 0);
        assert(fromZeroIndex < savedLines_.size());
        const auto start = savedLines_.begin();
        const auto end = start + fromZeroIndex;
        std::for_each(start, end, [this](const QString &line) { flushScriptLine(line); });
        savedLines_.erase(start, end);
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}

ScriptWriter::~ScriptWriter() noexcept
{
    if (scriptCancelled_) {
        script_.close();
        const auto filePath = script_.fileName();
        if (QFile::exists(filePath)) {
            QFile::remove(filePath);
        }
        return;
    }

    flushSavedLines();
    switch (recordSettings_.scriptWriteMode) {
    case common::ScriptWriteMode::NewScript: {
        flushScriptLine("}", 0);
        script_.close();
        break;
    }
    case common::ScriptWriteMode::UpdateScript: {
        for (const auto &line : savedLines_) {
            flushScriptLine(line);
        }
        script_.close();

        const auto &originalScriptPath = recordSettings_.scriptPath;
        if (QFile::exists(originalScriptPath)) {
            QFile::remove(originalScriptPath);
        }
        script_.rename(originalScriptPath);
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}

void ScriptWriter::handleNewLine(const QString &scriptLine) noexcept
{
    if (linesHandler_.repeatingLine != scriptLine) {
        flushSavedLines();
    }
    const auto isNewLine = linesHandler_.registerLine(scriptLine);
    if (isNewLine) {
        for (const auto &line : linesHandler_.cutLine) {
            if (line.startsWith('/')) {
                continue;
            }
            const auto bracketIndex = line.indexOf('(');
            assert(bracketIndex != -1);
            emit newScriptCommandDetected(line.left(bracketIndex));
            break;
        }
    }
}

void ScriptWriter::handleNewComment(const QString &comment) noexcept
{
    flushSavedLines();

    const auto commentLines = comment.trimmed().split('\n');
    const auto blockComment = commentLines.size() >= recordSettings_.blockCommentMinimumCount;

    if (blockComment) {
        flushScriptLine("/*");
    }
    for (const auto &line : commentLines) {
        const auto commentLine = QStringLiteral("%1%2").arg(blockComment ? "" : "// ").arg(line);
        flushScriptLine(commentLine);
    }
    if (blockComment) {
        flushScriptLine("*/");
    }
}

void ScriptWriter::handleNewMetaPropertyVerification(
    const QString &objectPath,
    const std::vector<std::pair<QString, QString>> &verifications) noexcept
{
    assert(!objectPath.isEmpty());
    flushSavedLines();

    for (const auto &verification : verifications) {
        const auto line = QStringLiteral("verify('%1', '%2', '%3');")
                              .arg(objectPath)
                              .arg(verification.first)
                              .arg(verification.second);
        flushScriptLine(std::move(line));
    }
}

void ScriptWriter::flushSavedLines() noexcept
{
    if (linesHandler_.cycleReady()) {
        flushScriptLine(linesHandler_.forStatement());
        for (const auto &line : linesHandler_.cutLine) {
            flushScriptLine(line, 2);
        }
        flushScriptLine("}");
    }
    else {
        for (int i = 0; i < linesHandler_.count; i++) {
            for (const auto &line : linesHandler_.cutLine) {
                flushScriptLine(line);
            }
        }
    }
    linesHandler_.clear();
}

void ScriptWriter::flushScriptLine(const QString &scriptLine, int indentMultiplier) noexcept
{
    assert(scriptStream_.device() != nullptr && scriptStream_.status() == QTextStream::Ok);
    if (scriptLine.isEmpty()) {
        scriptStream_ << Qt::endl;
    }
    else {
        scriptStream_ << QString(recordSettings_.indentWidth * indentMultiplier, ' ')
                      << scriptLine.trimmed() << Qt::endl;
    }
    scriptStream_.flush();
}
} // namespace QtAda::inprocess
