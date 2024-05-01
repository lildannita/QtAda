#pragma once

#include <QMainWindow>

#include "Settings.hpp"

QT_BEGIN_NAMESPACE
class QSettings;
class QFileInfo;
class QStandardItemModel;
class QStandardItem;
QT_END_NAMESPACE

namespace Ui {
class MainGui;
}

namespace QtAda::gui {
class CustomStandardItem;

class MainGui final : public QMainWindow {
    Q_OBJECT
public:
    MainGui(const QString &projectPath, QWidget *parent = nullptr);
    ~MainGui();

private slots:
    void addNewFileToProject(bool isNewFileMode, bool isScript) noexcept;
    void showProjectTreeContextMenu(const QPoint &pos) noexcept;

    void runScript(const QString &path) noexcept
    {
    }
    void openInEditor(const QString &path) noexcept
    {
    }
    void removeFromProject(const QString &path, bool isScript) noexcept;
    void removeDirFromProject(const QString &path) noexcept;
    void renameFile(QStandardItemModel *model, const QModelIndex &index) noexcept;
    void deleteFile(const QString &path, bool isScript) noexcept;
    void openExternally(const QString &path) noexcept;
    void showInFolder(const QString &path) noexcept;
    void openFolder(const QString &path) noexcept;
    void executeApplication(const QString &path) noexcept;

private:
    Ui::MainGui *ui = nullptr;
    bool uiInitialized_ = false;
    bool saveProjectFileOnExit_ = true;

    //! TODO: возможно, лучше не хранить QSettings открытым, так как иногда
    //! "внешние" изменения могут быть проигнорированы и перезаписаны (например,
    //! при изменении пути к тестируемому файлу) - и непонятно, хорошо ли это...
    QSettings *project_ = nullptr;
    QStringList lastScripts_;
    QStringList lastSources_;

    RecordSettings recordSettings_;
    ExecuteSettings executeSettings_;

    void configureProject(const QString &projectPath) noexcept;
    void updateProjectFileView(bool isExternal) noexcept;
    void configureSubTree(CustomStandardItem *rootItem, const QString &projectDirPath,
                          bool isScriptsTree) noexcept;
    QStringList getAccessiblePaths(const QFileInfo &projectInfo, bool isScripts) noexcept;

    void doRenameFile(QStandardItemModel *model, QStandardItem *rawItem,
                      const QString &oldName) noexcept;

    void saveGuiParamsToProjectFile() noexcept;
    void setGuiParamsFromProjectFile() noexcept;

    bool event(QEvent *event) override;
};
} // namespace QtAda::gui
