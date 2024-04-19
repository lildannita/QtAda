#include "ScriptWriter.hpp"

#include "utils/Tools.hpp"

namespace QtAda::core {
ScriptWriter::ScriptWriter(const GenerationSettings &settings, QObject *parent) noexcept
    : QObject{ parent }
    , generationSettings_{ settings }
{
    assert(generationSettings_.isInit());

    QFileInfo scriptInfo(generationSettings_.scriptPath);
    const auto scriptDir = scriptInfo.absoluteDir();
    assert(scriptDir.exists() && scriptDir.isReadable());
    assert(scriptInfo.suffix() == "js");

    script_.setFileName(generationSettings_.scriptPath);

    switch (settings.writeMode) {
    case ScriptWriteMode::NewScript: {
        bool isOpen = script_.open(QIODevice::WriteOnly | QIODevice::Truncate);
        assert(isOpen == true);
        scriptStream_.setDevice(&script_);
        flushScriptLine("function test() {", 0);
        break;
    }
    case ScriptWriteMode::UpdateScript: {
        bool isOpen = script_.open(QIODevice::ReadOnly);
        assert(isOpen == true);

        QTextStream readScript(&script_);
        while (!readScript.atEnd()) {
            savedLines_.push_back(readScript.readLine());
        }
        script_.close();

        assert(generationSettings_.appendLineIndex < savedLines_.size());

        isOpen = script_.open(QIODevice::WriteOnly | QIODevice::Truncate);
        assert(isOpen == true);
        scriptStream_.setDevice(&script_);

        const auto start = savedLines_.begin();
        const auto end = start + generationSettings_.appendLineIndex;
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
    if (generationSettings_.writeMode == ScriptWriteMode::NewScript) {
        flushScriptLine("}", 0);
    }
    else {
        for (const auto &line : savedLines_) {
            writeNewLine(line);
        }
    }
    script_.close();
}

void ScriptWriter::writeNewLine(const QString &srciptLine) noexcept
{
    auto lines = tools::cutLine(srciptLine);
    for (const auto &line : lines) {
        flushScriptLine(line, generationSettings_.indentWidth);
    }
}

void ScriptWriter::flushScriptLine(const QString &line, int indentWidth) noexcept
{
    assert(indentWidth >= 0);
    assert(scriptStream_.device() && scriptStream_.status() == QTextStream::Ok);
    scriptStream_ << QString(indentWidth, ' ') << line << Qt::endl;
}
} // namespace QtAda::core
