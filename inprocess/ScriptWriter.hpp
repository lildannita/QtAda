#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <vector>
#include <optional>

#include "GenerationSettings.hpp"
#include "InprocessTools.hpp"

namespace QtAda::inprocess {
class ScriptWriter final : public QObject {
    Q_OBJECT
public:
    explicit ScriptWriter(const common::GenerationSettings &settings,
                          QObject *parent = nullptr) noexcept;
    ~ScriptWriter() noexcept;

signals:
    void newScriptCommandDetected(const QString &command);

public slots:
    void handleNewLine(const QString &scriptLine) noexcept;
    void handleNewComment(const QString &comment) noexcept;
    void handleNewMetaPropertyVerification(
        const QString &objectPath,
        const std::vector<std::pair<QString, QString>> &verifications) noexcept;
    void handleCancelledScript() noexcept
    {
        scriptCancelled_ = true;
    }

private:
    struct LinesHandler final {
        const bool needToGenerateCycle;
        const int cycleMinimumCount;
        int count = 0;
        QString repeatingLine = QString();
        std::vector<QString> cutLine;

        LinesHandler(bool need, int minimum) noexcept
            : needToGenerateCycle{ need }
            , cycleMinimumCount{ minimum }
        {
        }

        void clear() noexcept
        {
            count = 0;
            repeatingLine.clear();
        }

        bool cycleReady() const noexcept
        {
            return needToGenerateCycle && count >= cycleMinimumCount;
        }

        bool registerLine(const QString &line) noexcept
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
            cutLine = tools::cutLine(repeatingLine);
            return true;
        }

        QString forStatement() const noexcept
        {
            return QStringLiteral("for (let i = 0; i < %1; i++) {").arg(count);
        }
    } linesHandler_;

    const common::GenerationSettings generationSettings_;

    QFile script_;
    QTextStream scriptStream_;
    std::vector<QString> savedLines_;
    bool scriptCancelled_ = false;

    void flushSavedLines() noexcept;
    void flushScriptLine(const QString &line, int indentMultiplier = 1) noexcept;
};
} // namespace QtAda::inprocess
