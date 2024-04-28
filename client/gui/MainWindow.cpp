#include "MainWindow.hpp"

#include "ui_MainWindow.h"

namespace QtAda::gui {
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
} // namespace QtAda::gui
