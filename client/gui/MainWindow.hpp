#pragma once

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

namespace QtAda::gui {
class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
};
} // namespace QtAda::gui
