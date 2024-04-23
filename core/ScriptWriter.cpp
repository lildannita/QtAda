#include "ScriptWriter.hpp"

#include "utils/Tools.hpp"
#include "utils/FilterUtils.hpp"

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
        flushScriptLine("function test() {", 0);
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
        flushScriptLine("}", 0);
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

void ScriptWriter::flushSavedLines() noexcept
{
    if (linesHandler_.cycleReady()) {
        flushScriptLine(linesHandler_.forStatement());
        const auto lines = tools::cutLine(linesHandler_.repeatingLine);
        for (const auto &line : lines) {
            flushScriptLine(line, 2);
        }
        flushScriptLine("}");
    }
    else {
        for (int i = 0; i < linesHandler_.count; i++) {
            const auto lines = tools::cutLine(linesHandler_.repeatingLine);
            for (const auto &line : lines) {
                flushScriptLine(line);
            }
        }
    }
}

void ScriptWriter::flushScriptLine(const QString &scriptLine, int indentMultiplier) noexcept
{
    assert(scriptStream_.device() != nullptr && scriptStream_.status() == QTextStream::Ok);
    if (scriptLine.isEmpty()) {
        scriptStream_ << Qt::endl;
    }
    else {
        scriptStream_ << QString(generationSettings_.indentWidth() * indentMultiplier, ' ')
                      << scriptLine.trimmed() << Qt::endl;
    }
    scriptStream_.flush();
}

void ScriptWriter::handleNewMetaPropertyVerification(
    const QObject *object, const std::vector<std::pair<QString, QString>> &verifications) noexcept
{
    flushSavedLines();

    assert(object != nullptr);
    const auto path = utils::objectPath(object);

    for (const auto &verification : verifications) {
        const auto line = QStringLiteral("verify('%1', '%2', '%3');")
                              .arg(path)
                              .arg(verification.first)
                              .arg(verification.second);
        flushScriptLine(std::move(line));
    }
}
} // namespace QtAda::core
