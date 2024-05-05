#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <vector>
#include <optional>

#include "Settings.hpp"
#include "InprocessTools.hpp"

namespace QtAda::inprocess {
class ScriptWriter final : public QObject {
    Q_OBJECT
public:
    explicit ScriptWriter(const RecordSettings &settings, QObject *parent = nullptr) noexcept;
    ~ScriptWriter() noexcept;

    void finishScript(bool isCancelled) noexcept;

signals:
    void newScriptCommandDetected(const QString &command);

public slots:
    void handleNewLine(const QString &scriptLine) noexcept;
    void handleNewComment(const QString &comment) noexcept;
    void handleNewMetaPropertyVerification(
        const QString &objectPath,
        const std::vector<std::pair<QString, QString>> &verifications) noexcept;

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

        QString forStatement() const noexcept
        {
            return QStringLiteral("for (let i = 0; i < %1; i++) {").arg(count);
        }

        bool registerLine(const QString &line) noexcept;
    } linesHandler_;

    const RecordSettings recordSettings_;

    QFile script_;
    QTextStream scriptStream_;
    std::vector<QString> savedLines_;
    bool scriptFinished_ = false;

    void flushSavedLines() noexcept;
    void flushScriptLine(const QString &line, int indentMultiplier = 1,
                         bool trimNeed = true) noexcept;
};
} // namespace QtAda::inprocess
