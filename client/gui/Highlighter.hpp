#pragma once

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>

namespace QtAda::gui {
class Highlighter final : public QSyntaxHighlighter {
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = nullptr) noexcept;

protected:
    void highlightBlock(const QString &text) noexcept override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> rules;

    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
};
} // namespace QtAda::gui
