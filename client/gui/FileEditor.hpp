#pragma once

#include <QPlainTextEdit>

#include "Settings.hpp"
#include "GuiTools.hpp"

QT_BEGIN_NAMESPACE
class QTabWidget;
QT_END_NAMESPACE

namespace QtAda::gui {
class FileEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    FileEditor(const QString &filePath, int role, QTabWidget *editorsTabWidget,
               QWidget *parent = nullptr) noexcept;

    bool readFile() noexcept;
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

    QTabWidget *editorsTabWidget_ = nullptr;

    RecordSettings recordSettings_;
    ExecuteSettings executeSettings_;

    bool isChanged_ = false;
    bool fileWasRead_ = false;

    void updateEditorTabName() noexcept;
};
} // namespace QtAda::gui
