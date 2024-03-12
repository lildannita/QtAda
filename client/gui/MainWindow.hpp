#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace QtAda {
namespace launcher {
    class Launcher;
}

namespace gui {
    class MainWindow : public QMainWindow {
        Q_OBJECT
    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

    private slots:
        void on_pushButton_clicked();

    private:
        Ui::MainWindow *ui;
        launcher::Launcher *launcher;
    };
} // namespace gui
} // namespace QtAda
