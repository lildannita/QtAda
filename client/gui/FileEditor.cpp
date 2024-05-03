#include "FileEditor.hpp"

#include <QShortcut>
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QTabWidget>

#include "Paths.hpp"

namespace QtAda::gui {
FileEditor::FileEditor(const QString &filePath, int role, QTabWidget *editorsTabWidget,
                       QWidget *parent) noexcept
    : QPlainTextEdit{ parent }
    , editorsTabWidget_{ editorsTabWidget }
    , filePath_{ filePath }
    , role_{ role }
{
    assert(!filePath_.isEmpty());
    assert(editorsTabWidget_ != nullptr);

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
    assert(!fileWasRead_);
    fileWasRead_ = true;

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
