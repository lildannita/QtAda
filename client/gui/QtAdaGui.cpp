#include "QtAdaGui.hpp"

#include <QSettings>
#include <QString>

#include "ui_QtAdaGui.h"

namespace QtAda::gui {
MainWindow::MainWindow(const QString &projectPath, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::QtAdaGui)
{
    ui->setupUi(this);

    // Так как в Qt Designer нельзя добавить Spacer в QToolBar, то
    // делаем это вручную
    auto *toolSpacer = new QWidget(this);
    toolSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->toolBar->addWidget(toolSpacer);
    ui->toolBar->addAction(ui->actionRunCurrent);
    ui->toolBar->addAction(ui->actionRunAll);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::configureProject(const QString &projectPath) noexcept
{
    if (project_ != nullptr) {
        project_->deleteLater();
    }
    project_ = new QSettings(projectPath, QSettings::IniFormat);
}

} // namespace QtAda::gui
