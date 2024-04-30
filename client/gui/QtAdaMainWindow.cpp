#include "QtAdaMainWindow.hpp"

#include <QMessageBox>
#include <QSettings>
#include <QStandardItemModel>
#include <QString>

#include "ui_QtAdaGui.h"
#include "Paths.hpp"
#include "GuiTools.hpp"

namespace QtAda::gui {
QtAdaMainWindow::QtAdaMainWindow(const QString &projectPath, QWidget *parent)
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

    auto *fileNotOpenedLabel = new QLabel(this);
    fileNotOpenedLabel->setAlignment(Qt::AlignCenter);
    fileNotOpenedLabel->setText(
        "No files are open.\nSelect the file in the project tree on the left.");
    auto *contentLayout = new QVBoxLayout(ui->contentWidget);
    contentLayout->addWidget(fileNotOpenedLabel);

    configureProject(projectPath);
}

QtAdaMainWindow::~QtAdaMainWindow()
{
    if (project_ != nullptr) {
        saveSizesToProjectFile();
        project_->deleteLater();
        project_ = nullptr;
    }

    delete ui;
}

void QtAdaMainWindow::configureProject(const QString &projectPath) noexcept
{
    if (project_ != nullptr) {
        saveSizesToProjectFile();
        project_->deleteLater();
    }
    project_ = new QSettings(projectPath, QSettings::IniFormat);

    updateProjectFileView();
    setSizesFromProjectFile();
}

void QtAdaMainWindow::updateProjectFileView() noexcept
{
    assert(project_ != nullptr);

    auto *projectFilesModel = new QStandardItemModel(this);

    const auto projectFileInfo = QFileInfo(project_->fileName());
    const auto projectOk = projectFileInfo.exists() && projectFileInfo.isFile()
                           && projectFileInfo.isReadable() && projectFileInfo.isWritable();

    if (!projectOk) {
        QMessageBox::critical(this, paths::QTADA_ERROR_HEADER,
                              QStringLiteral("The project file not accessible."));
        //! TODO: нужно придумать как "филиграннее" обрабатывать эту ситуацию
        QApplication::exit(1);
        return;
    }

    const auto appPath = project_->value(paths::PROJECT_APP_PATH, "").toString().trimmed();
    const auto appPathCheck = tools::checkProjectAppPath(appPath);
    if (appPathCheck != AppPathCheck::Ok) {
        QMessageBox::critical(this, paths::QTADA_ERROR_HEADER,
                              QStringLiteral("The testing application not accessible."));
        //! TODO: нужно придумать как "филиграннее" обрабатывать эту ситуацию
        QApplication::exit(1);
        return;
    }

    const auto projectDir = projectFileInfo.dir();
    auto *rootProjectDir = new QStandardItem(projectDir.dirName());
    rootProjectDir->setData(projectDir.absolutePath(), Roles::DirRole);
    rootProjectDir->setIcon(QIcon(":/icons/root_dir.svg"));
    rootProjectDir->setSelectable(false);
    projectFilesModel->appendRow(rootProjectDir);

    auto *projectViewItem = new QStandardItem(projectFileInfo.fileName());
    projectViewItem->setData(projectFileInfo.path(), Roles::ScriptRole);
    projectViewItem->setIcon(QIcon(":/icons/project.svg"));
    rootProjectDir->appendRow(projectViewItem);

    auto *scriptsDirItem = new QStandardItem("Test Scripts");
    scriptsDirItem->setIcon(QIcon(":/icons/source_dir.svg"));
    scriptsDirItem->setSelectable(false);
    rootProjectDir->appendRow(scriptsDirItem);

    auto *sourceDirItem = new QStandardItem("Source Files");
    sourceDirItem->setIcon(QIcon(":/icons/source_dir.svg"));
    sourceDirItem->setSelectable(false);
    rootProjectDir->appendRow(sourceDirItem);

    const auto appFileInfo = QFileInfo(appPath);
    auto *appItem = new QStandardItem(appFileInfo.fileName());
    appItem->setData(QUrl(appFileInfo.absoluteFilePath()), Roles::TestAppRole);
    appItem->setIcon(QIcon(":/icons/test_app.svg"));
    projectFilesModel->appendRow(appItem);

    auto *prevModel = ui->projectFilesView->model();
    if (prevModel == nullptr || prevModel->rowCount() == 0) {
        ui->projectFilesView->setModel(projectFilesModel);
    }

    ui->projectFilesView->expandAll();
}

void QtAdaMainWindow::saveSizesToProjectFile() noexcept
{
    assert(project_ != nullptr);

    project_->beginGroup(paths::PROJECT_SIZES_GROUP);
    QVariantList contentSizes;
    for (const auto &width : ui->contentSplitter->sizes()) {
        contentSizes.append(width);
    }
    assert(contentSizes.size() == 3);
    project_->setValue(paths::PROJECT_CONTENT_SIZES, contentSizes);

    QVariantList mainSizes;
    for (const auto &height : ui->mainSplitter->sizes()) {
        mainSizes.append(height);
    }
    assert(mainSizes.size() == 2);
    project_->setValue(paths::PROJECT_MAIN_SIZES, mainSizes);
    project_->endGroup();
    project_->sync();
}

void QtAdaMainWindow::setSizesFromProjectFile() noexcept
{
    assert(project_ != nullptr);

    project_->beginGroup(paths::PROJECT_SIZES_GROUP);
    const auto contentProjectSizes = project_->value(paths::PROJECT_CONTENT_SIZES, {}).toList();
    const auto mainProjectSizes = project_->value(paths::PROJECT_MAIN_SIZES, {}).toList();
    project_->endGroup();

    if (contentProjectSizes.isEmpty() || contentProjectSizes.size() != 3) {
        ui->contentSplitter->setSizes({ -1, ui->contentWidget->maximumWidth(), -1 });
    }
    else {
        QList<int> contentSizes;
        for (const auto &width : contentProjectSizes) {
            if (width.canConvert<int>()) {
                contentSizes.append(width.toInt());
            }
            else {
                contentSizes.append(-1);
            }
        }
        ui->contentSplitter->setSizes(contentSizes);
    }

    if (mainProjectSizes.isEmpty() || mainProjectSizes.size() != 2) {
        ui->mainSplitter->setSizes({ ui->contentSplitter->maximumHeight(), -1 });
    }
    else {
        QList<int> mainSizes;
        for (const auto &height : mainProjectSizes) {
            if (height.canConvert<int>()) {
                mainSizes.append(height.toInt());
            }
            else {
                mainSizes.append(-1);
            }
        }
        ui->mainSplitter->setSizes(mainSizes);
    }
}

} // namespace QtAda::gui
