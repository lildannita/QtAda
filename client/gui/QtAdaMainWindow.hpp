#pragma once

#include <QMainWindow>

#include "Settings.hpp"

QT_BEGIN_NAMESPACE
class QSettings;
class QStandardItem;
class QFileInfo;
QT_END_NAMESPACE

namespace Ui {
class QtAdaGui;
}

namespace QtAda::gui {
class QtAdaMainWindow final : public QMainWindow {
    Q_OBJECT
public:
    enum Roles {
        ScriptRole = Qt::UserRole,
        SourceRole,
        ProjectRole,
        TestAppRole,
        DirRole,
        None,
    };

    QtAdaMainWindow(const QString &projectPath, QWidget *parent = nullptr);
    ~QtAdaMainWindow();

private:
    Ui::QtAdaGui *ui = nullptr;
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
    void configureSubTree(QStandardItem *rootItem, const QString &projectDirPath,
                          bool isScriptsTree) noexcept;
    QStringList getAccessiblePaths(const QFileInfo &projectInfo, bool isScripts) noexcept;

    void saveGuiParamsToProjectFile() noexcept;
    void setGuiParamsFromProjectFile() noexcept;

    bool event(QEvent *event) override;
};
} // namespace QtAda::gui
