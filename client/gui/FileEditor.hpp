#pragma once

#include <QPlainTextEdit>

#include "Settings.hpp"
#include "GuiTools.hpp"

QT_BEGIN_NAMESPACE
class QTabWidget;
QT_END_NAMESPACE

namespace QtAda::gui {
// Способ превращения из обычного QPlainTextEdit в "настоящий" текстовый редактор:
// code.qt.io/cgit/qt/qtbase.git/tree/examples/widgets/widgets/codeeditor?h=5.15
class Editor : public QPlainTextEdit {
    Q_OBJECT
public:
    Editor(QAction *lineWrapAction, QWidget *parent = nullptr) noexcept;

    void lineNumberAreaPaintEvent(QPaintEvent *event) noexcept;
    int lineNumberAreaWidth() const noexcept;

    int lastHighlitedLine() const noexcept
    {
        return lastHighlitedLine_;
    }

protected:
    void resizeEvent(QResizeEvent *event) noexcept override;
    void wheelEvent(QWheelEvent *event) noexcept override;

private slots:
    void updateLineNumberAreaWidth() noexcept;
    void highlightCurrentLine() noexcept;
    void updateLineNumberArea(const QRect &rect, int dy) noexcept;

    void updateWrapMode() noexcept;

private:
    QWidget *lineNumberArea_;
    QAction *lineWrapAction_ = nullptr;

    int lastHighlitedLine_ = -1;
};

class LineNumberArea final : public QWidget {
public:
    LineNumberArea(Editor *editor) noexcept
        : QWidget(editor)
        , editor_(editor)
    {
        assert(editor != nullptr);
    }

    QSize sizeHint() const noexcept override
    {
        return QSize(editor_->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) noexcept override
    {
        editor_->lineNumberAreaPaintEvent(event);
    }

private:
    Editor *editor_ = nullptr;
};

class Highlighter;

class FileEditor final : public Editor {
    Q_OBJECT
public:
    FileEditor(const QString &filePath, int role, QTabWidget *editorsTabWidget,
               QAction *lineWrapAction, QWidget *parent = nullptr) noexcept;

    bool readFile() noexcept;
    bool reReadFile() noexcept;
    bool closeFile(bool needToConfirm = true) noexcept;
    void updateFilePath(const QString &filePath) noexcept;

    QString filePath() const noexcept
    {
        return filePath_;
    }
    int role() const noexcept
    {
        return role_;
    }
    bool isChanged() const noexcept
    {
        return isChanged_;
    }

    int lineCount() const noexcept
    {
        return this->blockCount();
    }

    void setSettings(const std::pair<RecordSettings, ExecuteSettings> &settings) noexcept
    {
        assert(role_ == FileRole::ScriptRole);
        recordSettings_ = settings.first;
        executeSettings_ = settings.second;
    }
    std::pair<RecordSettings, ExecuteSettings> getSettings() const noexcept
    {
        assert(role_ == FileRole::ScriptRole);
        return { recordSettings_, executeSettings_ };
    }
    RecordSettings getRecordSettings() const noexcept
    {
        assert(role_ == FileRole::ScriptRole);
        return recordSettings_;
    }
    ExecuteSettings getExecuteSettings() const noexcept
    {
        assert(role_ == FileRole::ScriptRole);
        return executeSettings_;
    }

signals:
    void projectFileHasChanged();
    void lineCountChanged(int lineCount);

public slots:
    void saveFile() noexcept;

private slots:
    void handleFileChange() noexcept;

private:
    QString filePath_;
    const int role_ = -1;

    Highlighter *highlighter_ = nullptr;
    QTabWidget *editorsTabWidget_ = nullptr;

    RecordSettings recordSettings_;
    ExecuteSettings executeSettings_;

    bool isChanged_ = false;

    void updateEditorTabName() noexcept;
};
} // namespace QtAda::gui
