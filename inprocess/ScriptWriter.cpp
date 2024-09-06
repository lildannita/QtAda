#include "ScriptWriter.hpp"

#include <QFileInfo>
#include <QDir>

#include "Paths.hpp"

namespace QtAda::inprocess {
static std::vector<QString> doCutLine(const QString &line) noexcept
{
    std::vector<int> indices;
    int index = 0;
    while ((index = line.indexOf('\n', index)) != -1) {
        if (index == 0 || line[index - 1] != '\\') {
            indices.push_back(index);
        }
        index += 1;
    }

    std::vector<QString> result;
    index = 0;
    for (const auto &position : indices) {
        result.push_back(line.mid(index, position - index));
        index = position + 1;
    }

    if (index < line.length()) {
        result.push_back(line.mid(index));
    }

    return result;
}

bool ScriptWriter::LinesHandler::registerLine(const QString &line) noexcept
{
    if (line.isEmpty()) {
        return false;
    }

    if (line == repeatingLine) {
        count++;
        return false;
    }

    count = 1;
    repeatingLine = line;
    cutLine = doCutLine(repeatingLine);
    return true;
}

ScriptWriter::ScriptWriter(const RecordSettings &settings, QObject *parent) noexcept
    : QObject{ parent }
    , recordSettings_{ settings }
    , linesHandler_{ settings.needToGenerateCycle, settings.cycleMinimumCount }
{
    QFileInfo scriptInfo(recordSettings_.scriptPath);
    const auto scriptDir = scriptInfo.absoluteDir();
    assert(scriptDir.exists() && scriptDir.isReadable());
    assert(scriptInfo.suffix() == "js");

    const auto tmpScriptPath
        = QStringLiteral("%1.%2").arg(recordSettings_.scriptPath).arg(paths::PROJECT_TMP_SUFFIX);

    switch (settings.scriptWriteMode) {
    case ScriptWriteMode::NewScript: {
        script_.setFileName(tmpScriptPath);
        bool isOpen = script_.open(QIODevice::WriteOnly | QIODevice::Truncate);
        assert(isOpen == true);
        scriptStream_.setDevice(&script_);
        flushScriptLine("function test() {", 0);
        break;
    }
    case ScriptWriteMode::UpdateScript: {
        assert(scriptInfo.exists());

        QFile originalScript(recordSettings_.scriptPath);
        bool isOpen = originalScript.open(QIODevice::ReadOnly | QIODevice::Text);
        assert(isOpen == true);
        QTextStream readScript(&originalScript);
        while (!readScript.atEnd()) {
            savedLines_.push_back(readScript.readLine());
        }
        originalScript.close();

        script_.setFileName(tmpScriptPath);
        isOpen = script_.open(QIODevice::WriteOnly | QIODevice::Truncate);
        assert(isOpen == true);
        scriptStream_.setDevice(&script_);

        // Так как нумерация при задании параметров начинается с единицы
        auto fromZeroIndex = recordSettings_.appendLineIndex - 1;
        assert(fromZeroIndex >= 0);
        assert(fromZeroIndex < savedLines_.size());
        const auto start = savedLines_.begin();
        const auto end = start + fromZeroIndex;
        std::for_each(start, end, [this](const QString &line) { flushScriptLine(line, 0, false); });
        savedLines_.erase(start, end);
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}

ScriptWriter::~ScriptWriter() noexcept
{
    if (!scriptFinished_) {
        finishScript(true);
    }
}

void ScriptWriter::finishScript(bool isCancelled) noexcept
{
    if (scriptFinished_) {
        // Может быть такое, что пользователь успеет два раза
        // нажать на кнопку завершения скрипта
        return;
    }
    scriptFinished_ = true;

    if (isCancelled) {
        script_.close();
        if (script_.exists()) {
            script_.remove();
        }
        return;
    }

    flushSavedLines();
    switch (recordSettings_.scriptWriteMode) {
    case ScriptWriteMode::NewScript: {
        flushScriptLine("}\ntest();", 0);
        break;
    }
    case ScriptWriteMode::UpdateScript: {
        for (const auto &line : savedLines_) {
            flushScriptLine(line, 0, false);
        }
        break;
    }
    default:
        Q_UNREACHABLE();
    }

    script_.close();
    const auto &originalScriptPath = recordSettings_.scriptPath;
    if (QFile::exists(originalScriptPath)) {
        QFile::remove(originalScriptPath);
    }
    script_.rename(originalScriptPath);
}

void ScriptWriter::handleNewLine(const QString &scriptLine) noexcept
{
    if (linesHandler_.repeatingLine != scriptLine) {
        flushSavedLines();
    }
    const auto isNewLine = linesHandler_.registerLine(scriptLine);
    if (isNewLine) {
        for (const auto &line : linesHandler_.cutLine) {
            if (line.startsWith('/') || line.startsWith("let")) {
                continue;
            }
            const auto bracketIndex = line.indexOf('(');
            assert(bracketIndex > 0);
            emit newScriptCommandDetected(line.left(bracketIndex));
            break;
        }
    }
}

void ScriptWriter::handleNewComment(const QString &comment) noexcept
{
    flushSavedLines();

    const auto commentLines = comment.trimmed().split('\n');
    const auto blockComment = recordSettings_.blockCommentMinimumCount > 0
                              && commentLines.size() >= recordSettings_.blockCommentMinimumCount;

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
        const auto line = QStringLiteral("verify(%1, '%2', '%3');")
                              .arg(objectPath, verification.first, verification.second);
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

void ScriptWriter::flushScriptLine(const QString &scriptLine, int indentMultiplier,
                                   bool trimNeed) noexcept
{
    assert(scriptStream_.device() != nullptr && scriptStream_.status() == QTextStream::Ok);
    if (scriptLine.isEmpty()) {
        scriptStream_ << Qt::endl;
    }
    else {
        scriptStream_ << QString(recordSettings_.indentWidth * indentMultiplier, ' ')
                      << (trimNeed ? scriptLine.trimmed() : scriptLine) << Qt::endl;
    }
    scriptStream_.flush();
}
} // namespace QtAda::inprocess
