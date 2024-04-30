#pragma once

#include <QMainWindow>

#include "Settings.hpp"

QT_BEGIN_NAMESPACE
class QSettings;
class QString;
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

    QSettings *project_ = nullptr;

    RecordSettings recordSettings_;
    ExecuteSettings executeSettings_;

    void configureProject(const QString &projectPath) noexcept;
    void updateProjectFileView() noexcept;
    void configureSubTree(QStandardItem *rootItem, const QFileInfo &projectInfo,
                          bool isScriptsTree) noexcept;

    void saveSizesToProjectFile() noexcept;
    void setSizesFromProjectFile() noexcept;
};
} // namespace QtAda::gui
