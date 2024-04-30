#pragma once

#include <QMainWindow>

#include "Settings.hpp"

QT_BEGIN_NAMESPACE
class QSettings;
class QString;
class QStandardItemModel;
QT_END_NAMESPACE

namespace Ui {
class QtAdaGui;
}

namespace QtAda::gui {
class QtAdaMainWindow final : public QMainWindow {
    Q_OBJECT
public:
    QtAdaMainWindow(const QString &projectPath, QWidget *parent = nullptr);
    ~QtAdaMainWindow();

private:
    enum Roles {
        ScriptRole = Qt::UserRole,
        TestAppRole,
        DirRole,
        None,
    };

    Ui::QtAdaGui *ui = nullptr;

    QSettings *project_ = nullptr;

    RecordSettings recordSettings_;
    ExecuteSettings executeSettings_;

    void configureProject(const QString &projectPath) noexcept;

    void updateProjectFileView() noexcept;
    void saveSizesToProjectFile() noexcept;
    void setSizesFromProjectFile() noexcept;
};
} // namespace QtAda::gui
