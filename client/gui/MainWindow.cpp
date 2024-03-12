#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "Launcher.hpp"

namespace QtAda::gui {
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    if (launcher) {
        delete launcher;
    }
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    const auto path = ui->pathEdit->text();
    if (!path.isEmpty()) {
        launcher::UserLaunchOptions options;
        options.launchAppArguments << std::move(path);
        launcher = new launcher::Launcher(options);
        launcher->launch();
    }
}

} // namespace QtAda::gui
