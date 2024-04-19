#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <vector>
#include <optional>

#include "GenerationSettings.hpp"

namespace QtAda::core {
class ScriptWriter final : public QObject {
    Q_OBJECT
public:
    explicit ScriptWriter(const GenerationSettings &settings, QObject *parent = nullptr) noexcept;
    ~ScriptWriter() noexcept;

public slots:
    void writeNewLine(const QString &srciptLine) noexcept;

private:
    const GenerationSettings generationSettings_;
    QFile script_;
    QTextStream scriptStream_;
    std::vector<QString> savedLines_;

    void flushScriptLine(const QString &line, int indentWidth) noexcept;
};
} // namespace QtAda::core
