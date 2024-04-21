#include "ScriptWriter.hpp"

#include "utils/Tools.hpp"

namespace QtAda::core {
ScriptWriter::ScriptWriter(const GenerationSettings &settings, QObject *parent) noexcept
    : QObject{ parent }
    , generationSettings_{ settings }
    , linesHandler_{ settings.needToGenerateCycle(), settings.cycleMinimumCount() }
{
    QFileInfo scriptInfo(generationSettings_.scriptPath());
    const auto scriptDir = scriptInfo.absoluteDir();
    assert(scriptDir.exists() && scriptDir.isReadable());
    assert(scriptInfo.suffix() == "js");

    switch (settings.scriptWriteMode()) {
    case ScriptWriteMode::NewScript: {
        script_.setFileName(generationSettings_.scriptPath());
        bool isOpen = script_.open(QIODevice::WriteOnly | QIODevice::Truncate);
        assert(isOpen == true);
        scriptStream_.setDevice(&script_);
        writeScriptLine("function test() {", 0);
        break;
    }
    case ScriptWriteMode::UpdateScript: {
        assert(scriptInfo.exists());

        QFile originalScript(generationSettings_.scriptPath());
        bool isOpen = originalScript.open(QIODevice::ReadOnly);
        assert(isOpen == true);
        QTextStream readScript(&script_);
        while (!readScript.atEnd()) {
            savedLines_.push_back(readScript.readLine());
        }
        originalScript.close();

        script_.setFileName(generationSettings_.scriptPath() + ".qtada");
        isOpen = script_.open(QIODevice::WriteOnly | QIODevice::Truncate);
        assert(isOpen == true);
        scriptStream_.setDevice(&script_);

        assert(generationSettings_.appendLineIndex() < savedLines_.size());
        const auto start = savedLines_.begin();
        const auto end = start + generationSettings_.appendLineIndex();
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
    switch (generationSettings_.scriptWriteMode()) {
    case ScriptWriteMode::NewScript: {
        writeScriptLine("}", 0);
        script_.close();
        break;
    }
    case ScriptWriteMode::UpdateScript: {
        for (const auto &line : savedLines_) {
            flushScriptLine(line);
        }
        script_.close();

        const auto &originalScriptPath = generationSettings_.scriptPath();
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
    linesHandler_.registerLine(scriptLine);
}

void ScriptWriter::handleNewComment(const QString &comment) noexcept
{
    flushSavedLines();

    const auto commentLines = comment.trimmed().split('\n');
    const auto blockComment = commentLines.size() >= generationSettings_.blockCommentMinimumCount();

    if (blockComment) {
        writeScriptLine("/*");
    }
    for (const auto &line : commentLines) {
        const auto commentLine = QStringLiteral("%1%2").arg(blockComment ? "" : "// ").arg(line);
        writeScriptLine(commentLine);
    }
    if (blockComment) {
        writeScriptLine("*/");
    }
}

void ScriptWriter::flushSavedLines() noexcept
{
    if (linesHandler_.cycleReady()) {
        writeScriptLine(linesHandler_.forStatement());
        writeScriptLine(linesHandler_.repeatingLine, 2);
        writeScriptLine("}");
    }
    else {
        for (int i = 0; i < linesHandler_.count; i++) {
            writeScriptLine(linesHandler_.repeatingLine);
        }
    }
}

void ScriptWriter::writeScriptLine(const QString &scriptLine, int indentMultiplier) noexcept
{
    if (scriptLine.isEmpty()) {
        flushScriptLine(QString());
        return;
    }

    const auto lines = tools::cutLine(scriptLine);
    for (const auto &line : lines) {
        flushScriptLine(QString(generationSettings_.indentWidth() * indentMultiplier, ' ') + line);
    }
}

void ScriptWriter::flushScriptLine(const QString &scriptLine) noexcept
{
    assert(scriptStream_.device() != nullptr && scriptStream_.status() == QTextStream::Ok);
    scriptStream_ << scriptLine.trimmed() << Qt::endl;
    scriptStream_.flush();
}
} // namespace QtAda::core
