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
    void handleNewLine(const QString &scriptLine) noexcept;
    void handleNewComment(const QString &comment) noexcept;
    void handleNewMetaPropertyVerification(
        const QObject *object,
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

        void registerLine(const QString &line) noexcept
        {
            if (line.isEmpty()) {
                return;
            }

            if (line == repeatingLine) {
                count++;
            }
            else {
                count = 1;
                repeatingLine = line;
            }
        }

        QString forStatement() const noexcept
        {
            return QStringLiteral("for (let i = 0; i < %1; i++) {").arg(count);
        }
    } linesHandler_;

    const GenerationSettings generationSettings_;
    QFile script_;
    QTextStream scriptStream_;
    std::vector<QString> savedLines_;
    bool scriptCancelled_ = false;

    void flushSavedLines() noexcept;
    //    void writeScriptLine(const QString &line, int indentMultiplier = 1) noexcept;
    void flushScriptLine(const QString &line, int indentMultiplier = 1) noexcept;
};
} // namespace QtAda::core
