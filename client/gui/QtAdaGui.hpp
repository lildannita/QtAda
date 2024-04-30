#pragma once

#include <QMainWindow>

#include "Settings.hpp"

QT_BEGIN_NAMESPACE
class QSettings;
class QString;
QT_END_NAMESPACE

namespace Ui {
class QtAdaGui;
}

namespace QtAda::gui {
class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(const QString &projectPath, QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::QtAdaGui *ui = nullptr;

    QSettings *project_ = nullptr;

    RecordSettings recordSettings_;
    ExecuteSettings executeSettings_;

    void configureProject(const QString &projectPath) noexcept;
};
} // namespace QtAda::gui
