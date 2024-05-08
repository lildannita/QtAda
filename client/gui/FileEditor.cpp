#include "FileEditor.hpp"

#include <QApplication>
#include <QFontDatabase>
#include <QFont>
#include <QShortcut>
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QTabWidget>
#include <QPainter>
#include <QTextBlock>
#include <QAction>
#include <QSpinBox>

#include "Paths.hpp"
#include "Highlighter.hpp"

namespace QtAda::gui {
Editor::Editor(QAction *lineWrapAction, QSpinBox *lineIndexSpinBox, QWidget *parent) noexcept
    : QPlainTextEdit(parent)
    , lineWrapAction_{ lineWrapAction }
    , lineIndexSpinBox_{ lineIndexSpinBox }
{
    assert(lineWrapAction_ != nullptr);
    updateWrapMode();
    connect(lineWrapAction_, &QAction::triggered, this, &Editor::updateWrapMode);

    QFontDatabase fontDatabase;
    auto font = QApplication::font();
    if (fontDatabase.hasFamily("Hack")) {
        font.setFamily("Hack");
    }
    this->setFont(font);

    lineNumberArea_ = new LineNumberArea(this);

    connect(this, &Editor::blockCountChanged, this, &Editor::updateLineNumberAreaWidth);
    connect(this, &Editor::updateRequest, this, &Editor::updateLineNumberArea);
    connect(this, &Editor::cursorPositionChanged, this, &Editor::highlightCurrentLine);

    updateLineNumberAreaWidth();
    highlightCurrentLine();
}

int Editor::lineNumberAreaWidth() const noexcept
{
    int digits = 1;
    int max = qMax(1, this->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    const auto space = fontMetrics().horizontalAdvance(QLatin1Char('9')) * qMax(digits, 3) + 5;
    return space;
}

void Editor::updateLineNumberAreaWidth() noexcept
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    if (lineIndexSpinBox_ != nullptr) {
        lineIndexSpinBox_->setMaximum(this->blockCount());
    }
}

void Editor::updateLineNumberArea(const QRect &rect, int dy) noexcept
{
    if (dy != 0) {
        lineNumberArea_->scroll(0, dy);
    }
    else {
        lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth();
    }
}

void Editor::updateWrapMode() noexcept
{
    this->setLineWrapMode(lineWrapAction_->isChecked() ? LineWrapMode::WidgetWidth
                                                       : LineWrapMode::NoWrap);
}

void Editor::resizeEvent(QResizeEvent *event) noexcept
{
    QPlainTextEdit::resizeEvent(event);

    const auto cr = contentsRect();
    lineNumberArea_->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void Editor::highlightCurrentLine() noexcept
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(255, 219, 139, 20);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        auto textCursor = this->textCursor();
        lastHighlitedLine_ = textCursor.blockNumber() + 1;
        selection.cursor = textCursor;
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    this->setExtraSelections(extraSelections);
}

void Editor::lineNumberAreaPaintEvent(QPaintEvent *event) noexcept
{
    // Делаем текущие цвета на 20% темнее
    const auto baseColor = this->palette().base().color().darker(125);
    const auto textColor = this->palette().color(QPalette::Text).darker(125);

    QPainter painter(lineNumberArea_);
    painter.fillRect(event->rect(), baseColor);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(textColor);
            painter.drawText(-2, top, lineNumberArea_->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void Editor::wheelEvent(QWheelEvent *event) noexcept
{
    if (event->modifiers() != Qt::ControlModifier) {
        QPlainTextEdit::wheelEvent(event);
        return;
    }

    auto font = this->font();
    const auto currentFontSize = font.pointSize();
    if (event->angleDelta().y() > 0) {
        font.setPointSize(currentFontSize + 1);
    }
    else if (event->angleDelta().y() < 0) {
        font.setPointSize(currentFontSize - 1);
    }
    this->setFont(font);
    this->update();

    //! TODO: позже нужно будет изменять масштаб у всех остальных файлов,
    //! и сохранять последний масштаб в файл проекта
}

FileEditor::FileEditor(const QString &filePath, int role, QTabWidget *editorsTabWidget,
                       QAction *lineWrapAction, QSpinBox *lineIndexSpinBox,
                       QWidget *parent) noexcept
    : Editor{ lineWrapAction, lineIndexSpinBox, parent }
    , editorsTabWidget_{ editorsTabWidget }
    , filePath_{ filePath }
    , role_{ role }
{
    assert(!filePath_.isEmpty());
    assert(editorsTabWidget_ != nullptr);

    if (role_ == FileRole::ScriptRole) {
        highlighter_ = new Highlighter(this->document());
    }

    auto *shortcut = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(shortcut, &QShortcut::activated, this, &FileEditor::saveFile);
}

void FileEditor::saveFile() noexcept
{
    if (!isChanged_) {
        return;
    }
    QFile file(filePath_);
    assert(file.exists());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::warning(this, paths::QTADA_WARNING_HEADER,
                             QStringLiteral("Can't save file '%1'.").arg(filePath_));
        return;
    }
    QTextStream out(&file);
    out << this->toPlainText();
    file.close();
    isChanged_ = false;
    updateEditorTabName();

    switch (role_) {
    case FileRole::ProjectRole:
        emit projectFileHasChanged();
        break;
    case FileRole::ScriptRole:
        emit lineCountChanged(this->blockCount());
        break;
    default:
        break;
    }
}

void FileEditor::updateFilePath(const QString &filePath) noexcept
{
    filePath_ = filePath;
    updateEditorTabName();
}

bool FileEditor::readFile() noexcept
{
    //! TODO: нужно разобраться как различать текстовые файлы от бинарных

    QFile file(filePath_);
    assert(file.exists());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, paths::QTADA_WARNING_HEADER,
                             QStringLiteral("Can't read file '%1'.").arg(filePath_));
        return false;
    }
    this->setPlainText(QString::fromUtf8(file.readAll()));
    file.close();

    connect(this, &QPlainTextEdit::textChanged, this, &FileEditor::handleFileChange);
    return true;
}

bool FileEditor::reReadFile() noexcept
{
    assert(role_ == FileRole::ProjectRole || role_ == FileRole::ScriptRole);
    isChanged_ = false;
    disconnect(this, &QPlainTextEdit::textChanged, this, 0);
    updateEditorTabName();
    return readFile();
}

bool FileEditor::closeFile(bool needToConfirm) noexcept
{
    if (!isChanged_ || !needToConfirm) {
        isChanged_ = false;
        return true;
    }

    const auto confirm = QMessageBox::question(
        this, paths::QTADA_UNSAVED_CHANGES_HEADER,
        QStringLiteral("You have unsaved changes in file '%1'.\n"
                       "Do you want to save your changes?")
            .arg(filePath_),
        QMessageBox::No | QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);
    if (confirm == QMessageBox::Cancel) {
        return false;
    }
    if (confirm == QMessageBox::Yes) {
        saveFile();
    }
    isChanged_ = false;
    return true;
}

void FileEditor::handleFileChange() noexcept
{
    if (isChanged_) {
        return;
    }
    isChanged_ = true;
    updateEditorTabName();
}

void FileEditor::updateEditorTabName() noexcept
{
    for (int i = 0; i < editorsTabWidget_->count(); i++) {
        if (editorsTabWidget_->widget(i) == this) {
            editorsTabWidget_->setTabText(i, QFileInfo(filePath_).fileName()
                                                 + (isChanged_ ? " *" : ""));
            break;
        }
    }
}
} // namespace QtAda::gui
