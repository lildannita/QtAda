#pragma once

#include <QMainWindow>
#include <QStandardItem>
#include <QTextEdit>

#include "Settings.hpp"
#include "LaunchOptions.hpp"

QT_BEGIN_NAMESPACE
class QSettings;
class QFileInfo;
class QStandardItemModel;
class QLabel;
class QTabWidget;
QT_END_NAMESPACE

namespace Ui {
class MainGui;
}

namespace QtAda::launcher {
class Launcher;
}

namespace QtAda::gui {
class CustomStandardItem;
class FileEditor;

class MainGui final : public QMainWindow {
    Q_OBJECT
public:
    MainGui(const QString &projectPath, QWidget *parent = nullptr);
    ~MainGui();

signals:
    void projectFileHasChanged();

private slots:
    void addNewFileToProject(bool isNewFileMode, bool isScript) noexcept;
    void showProjectTreeContextMenu(const QPoint &pos) noexcept;

    void openFile(const QModelIndex &index) noexcept;
    void closeFileInEditor(int index) noexcept;
    void checkIfCurrentTabIsScript(int index) noexcept;

    void handleSettingsChange() noexcept;
    void handleLaunchSettingsChange() noexcept;

    void runScript(const QString &path) noexcept
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

    void startupScriptWriterLauncher(bool isUpdateMode) noexcept;

    void writeQtAdaErrMessage(const QString &msg) noexcept;
    void writeQtAdaOutMessage(const QString &msg) noexcept;
    void writeAppOutMessage(const QString &msg) noexcept;

private:
    using Settings = std::pair<RecordSettings, ExecuteSettings>;
    using ConstSettings = const std::pair<const RecordSettings &, const ExecuteSettings &> &;

    struct LaunchSettings final {
        QString workingDirectory;
        int timeoutValue;
    };

    Ui::MainGui *ui = nullptr;
    launcher::Launcher *launcher_ = nullptr;
    bool uiInitialized_ = false;
    bool saveProjectFileOnExit_ = true;

    QLabel *fileNotOpenedLabel_ = nullptr;
    QTabWidget *editorsTabWidget_ = nullptr;
    FileEditor *lastScriptEditor_ = nullptr;

    //! TODO: возможно, лучше не хранить QSettings открытым, так как иногда
    //! "внешние" изменения могут быть проигнорированы и перезаписаны (например,
    //! при изменении пути к тестируемому файлу) - и непонятно, хорошо ли это...
    QSettings *project_ = nullptr;
    QStringList lastScripts_;
    QStringList lastSources_;

    bool settingsChangeHandlerBlocked_ = false;
    bool launchSettingsChangeHandlerBlocked_ = false;

    Settings readCurrentSettings() const noexcept;
    void saveScriptSettings(const QString &path, ConstSettings settings) noexcept;
    void updateCurrentSettings(ConstSettings settings) noexcept;
    void updateCurrentLaunchSettings(const LaunchSettings &settings) noexcept;
    void updateScriptPathForSettings(const QString &oldPath,
                                     const QString &newPath = QString()) noexcept;

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
    void closeEvent(QCloseEvent *event) override;

    void flushProjectFile() noexcept;

    launcher::UserLaunchOptions getUserOptionsForLauncher(const QString &appPath,
                                                          const QString &scriptPath,
                                                          bool isRecordScript,
                                                          bool isUpdateMode = false) const noexcept;
    void prepareLogTextEdits(const QString &appPath, const QString &scriptPath, bool isStarting,
                             bool isFailedOnStart = false, int exitCode = 0) noexcept;
};
} // namespace QtAda::gui
