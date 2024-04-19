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

    script_.setFileName(generationSettings_.scriptPath());

    switch (settings.scriptWriteMode()) {
    case ScriptWriteMode::NewScript: {
        bool isOpen = script_.open(QIODevice::WriteOnly | QIODevice::Truncate);
        assert(isOpen == true);
        scriptStream_.setDevice(&script_);
        flushScriptLine("function test() {", 0);
        break;
    }
    case ScriptWriteMode::UpdateScript: {
        assert(scriptInfo.exists());
        bool isOpen = script_.open(QIODevice::ReadOnly);
        assert(isOpen == true);

        QTextStream readScript(&script_);
        while (!readScript.atEnd()) {
            savedLines_.push_back(readScript.readLine());
        }
        script_.close();

        assert(generationSettings_.appendLineIndex() < savedLines_.size());

        isOpen = script_.open(QIODevice::WriteOnly | QIODevice::Truncate);
        assert(isOpen == true);
        scriptStream_.setDevice(&script_);

        const auto start = savedLines_.begin();
        const auto end = start + generationSettings_.appendLineIndex();
        std::for_each(start, end,
                      [this](const QString &line) { scriptStream_ << line << Qt::endl; });
        savedLines_.erase(start, end);
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}

ScriptWriter::~ScriptWriter() noexcept
{
    flushSavedLines();
    if (generationSettings_.scriptWriteMode() == ScriptWriteMode::NewScript) {
        flushScriptLine("}", 0);
    }
    else {
        for (const auto &line : savedLines_) {
            scriptStream_ << line << Qt::endl;
        }
    }
    script_.close();
}

void ScriptWriter::handleNewLine(const QString &scriptLine) noexcept
{
    if (linesHandler_.repeatingLine != scriptLine) {
        flushSavedLines();
    }
    linesHandler_.registerLine(scriptLine);
}

void ScriptWriter::flushSavedLines() noexcept
{
    if (linesHandler_.cycleReady()) {
        flushScriptLine(linesHandler_.forStatement());
        flushScriptLine(linesHandler_.repeatingLine, 2);
        flushScriptLine("}");
    }
    else {
        for (int i = 0; i < linesHandler_.count; i++) {
            flushScriptLine(linesHandler_.repeatingLine);
        }
    }
}

void ScriptWriter::flushScriptLine(const QString &scriptLine, int indentMultiplier) noexcept
{
    assert(scriptStream_.device() && scriptStream_.status() == QTextStream::Ok);
    if (scriptLine.isEmpty()) {
        scriptStream_ << Qt::endl;
        return;
    }

    const auto lines = tools::cutLine(scriptLine);
    for (const auto &line : lines) {
        scriptStream_ << QString(generationSettings_.indentWidth() * indentMultiplier, ' ') << line
                      << Qt::endl;
    }
}
} // namespace QtAda::core
