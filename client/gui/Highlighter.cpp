#include "Highlighter.hpp"

//! TODO: сделано на коленке, нужно будет переделать

namespace QtAda::gui {
Highlighter::Highlighter(QTextDocument *parent) noexcept
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor("#569CD6"));
    keywordFormat.setFontWeight(QFont::Bold);
    const QStringList keywordPatterns
        = { "\\barguments\\b", "\\bbreak\\b",     "\\bcase\\b",       "\\bcatch\\b",
            "\\bcontinue\\b",  "\\bdefault\\b",   "\\bdelete\\b",     "\\bdo\\b",
            "\\belse\\b",      "\\beval\\b",      "\\bfinaly\\b",     "\\bfor\\b",
            "\\bfunction\\b",  "\\bif\\b",        "\\bin\\b",         "\\binstanceof\\b",
            "\\bnew\\b",       "\\breturn\\b",    "\\bswitch\\b",     "\\bthis\\b",
            "\\bthrow\\b",     "\\btry\\b",       "\\btypeof\\b",     "\\blet\\b",
            "\\bvar\\b",       "\\bvoid\\b",      "\\bwhile\\b",      "\\bwith\\b",
            "\\babstract\\b",  "\\bboolean\\b",   "\\bbyte\\b",       "\\bchar\\b",
            "\\bclass\\b",     "\\bconst\\b",     "\\bdebugger\\b",   "\\bdouble\\b",
            "\\benum\\b",      "\\bexport\\b",    "\\bextends\\b",    "\\bfinal\\b",
            "\\bfloat\\b",     "\\bgoto\\b",      "\\bimplements\\b", "\\bimport\\b",
            "\\bint\\b",       "\\binterface\\b", "\\blong\\b",       "\\bnative\\b",
            "\\bpackage\\b",   "\\bprivate\\b",   "\\bprotected\\b",  "\\bpublic\\b",
            "\\bshort\\b",     "\\bstatic\\b",    "\\bsuper\\b",      "\\bsynchronized\\b",
            "\\bthrows\\b",    "\\btransient\\b", "\\bvolatile\\b" };

    for (const auto &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        rules.append(rule);
    }

    QTextCharFormat functionFormat;
    functionFormat.setForeground(QColor("#DCDCAA"));
    functionFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\b\\w+(?=\\()");
    rule.format = functionFormat;
    rules.append(rule);

    QTextCharFormat verifyFormat;
    verifyFormat.setForeground(QColor("#FF8080"));
    verifyFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\bverify(?=\\()");
    rule.format = verifyFormat;
    rules.append(rule);

    QTextCharFormat bracketFormat;
    bracketFormat.setForeground(QColor("#FFD700"));
    bracketFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\{|\\}");
    rule.format = bracketFormat;
    rules.append(rule);

    QTextCharFormat parenthesisFormat;
    parenthesisFormat.setForeground(QColor("#DA70D6"));
    parenthesisFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\(|\\)");
    rule.format = parenthesisFormat;
    rules.append(rule);

    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor("#B5CEA8"));
    numberFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\b[+-]?[0-9]+(?:\\.[0-9]+)?\\b");
    rule.format = numberFormat;
    rules.append(rule);

    QTextCharFormat booleanFormat;
    booleanFormat.setForeground(QColor("#45C6D6"));
    booleanFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\b(?:true|false|null)\\b");
    rule.format = booleanFormat;
    rules.append(rule);

    QTextCharFormat quotationFormat;
    quotationFormat.setForeground(QColor("#CE9178"));
    quotationFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\"(?:\\\\.|[^\"])*\"|'(?:\\\\.|[^'])*'");
    rule.format = quotationFormat;
    rules.append(rule);

    singleLineCommentFormat.setForeground(QColor("#6A9955"));
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = singleLineCommentFormat;
    rules.append(rule);

    multiLineCommentFormat.setForeground(QColor("#6A9955"));
    rule.pattern
        = QRegularExpression("/\\*.*?\\*/", QRegularExpression::DotMatchesEverythingOption);
    rule.format = multiLineCommentFormat;
    rules.append(rule);
}

void Highlighter::highlightBlock(const QString &text) noexcept
{
    int startIndex = 0;
    int commentStartIndex = 0;

    //! TODO: главная проблема этого кода - комментарии в строковых литералах
    //! распознаются подсвечиваются как комментарии, а не как строки

    if (previousBlockState() != 1) {
        // Ищем начало комментария в текущем блоке
        startIndex = text.indexOf(QRegularExpression("/\\*"));
    }

    while (startIndex >= 0) {
        auto match = QRegularExpression("/\\*").match(text, startIndex);
        if (match.hasMatch()) {
            commentStartIndex = match.capturedStart();
            // Ищем конец комментария начиная с места найденного начала
            auto endMatch = QRegularExpression("\\*/").match(text, commentStartIndex);
            if (endMatch.hasMatch()) {
                // Если конец комментария найден, подсветить от начала до конца
                auto commentLength = endMatch.capturedEnd() - commentStartIndex;
                setFormat(commentStartIndex, commentLength, multiLineCommentFormat);
                // Продолжаем поиск следующего начала комментария после текущего конца
                startIndex = text.indexOf(QRegularExpression("/\\*"), endMatch.capturedEnd());
            }
            else {
                // Если конец комментария не найден, подсветить до конца блока и установить
                // состояние
                setFormat(commentStartIndex, text.length() - commentStartIndex,
                          multiLineCommentFormat);
                setCurrentBlockState(1);
                break;
            }
        }
        else {
            // Нет начала комментария
            startIndex = -1;
        }
    }

    if (previousBlockState() == 1 && startIndex == -1) {
        // Если блок начинается в состоянии комментария и не найдено новое начало комментария
        auto endMatch = QRegularExpression("\\*/").match(text);
        if (endMatch.hasMatch()) {
            // Если нашли конец комментария, подсветить от начала блока до конца комментария
            setFormat(0, endMatch.capturedEnd(), multiLineCommentFormat);
        }
        else {
            // Если конец так и не найден, весь блок является комментарием
            setFormat(0, text.length(), multiLineCommentFormat);
            setCurrentBlockState(1);
        }
    }

    for (const auto &rule : qAsConst(rules)) {
        auto matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            auto match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
} // namespace QtAda::gui
