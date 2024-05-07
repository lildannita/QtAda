#include "MainGui.hpp"

#include <QMessageBox>
#include <QSettings>
#include <QStandardItemModel>
#include <QString>
#include <QFileDialog>
#include <QDesktopServices>
#include <QProcess>
#include <QCloseEvent>
#include <QTime>
#include <set>
#include <memory>

#include "ui_MainGui.h"
#include "InitDialog.hpp"
#include "Paths.hpp"
#include "GuiTools.hpp"
#include "FileEditor.hpp"
#include "Launcher.hpp"

namespace QtAda::gui {
class CustomStandardItem : public QStandardItem {
public:
    CustomStandardItem(const QString &name, const QIcon &icon, bool isSelectable = true) noexcept
        : QStandardItem{ name }
    {
        this->setIcon(icon);
        this->setSelectable(isSelectable);
    }

    CustomStandardItem(const QString &name, const QVariant &value, const QIcon &icon, int role,
                       bool isSelectable = true) noexcept
        : CustomStandardItem{ name, icon, isSelectable }
    {
        role_ = role;
        this->setData(value, role_);
    }

    int role() const noexcept
    {
        return role_;
    }

private:
    int role_ = FileRole::None;
};

static CustomStandardItem *initFileItem(const QString &fileName, const QString &filePath,
                                        bool isScript) noexcept
{
    return new CustomStandardItem(
        fileName, filePath, isScript ? QIcon(":/icons/script.svg") : QIcon(":/icons/source.svg"),
        isScript ? FileRole::ScriptRole : FileRole::SourceRole);
}

static CustomStandardItem *initDirItem(const QString &dirName, const QString &dirPath,
                                       bool isSourceDir, bool isRootDir = false) noexcept
{
    if (!dirPath.isEmpty() && !isSourceDir) {
        return new CustomStandardItem(
            dirName, dirPath, isRootDir ? QIcon(":/icons/root_dir.svg") : QIcon(":/icons/dir.svg"),
            isRootDir ? FileRole::RootDirRole : FileRole::DirRole, false);
    }
    else {
        return new CustomStandardItem(dirName, QIcon(":/icons/source_dir.svg"), false);
    }
}

static void handleSubDirectories(const QString &projectDirPath, CustomStandardItem *rootItem,
                                 bool isScriptsTree,
                                 QMap<QString, CustomStandardItem *> *projectSubDirs,
                                 const QFileInfo &fileInfo) noexcept
{
    // Указатель на элемент папки в модели
    CustomStandardItem *subDirItem = nullptr;
    const auto fileDirPath = fileInfo.dir().absolutePath();

    // Проверка существования папки по такому пути
    if (!projectSubDirs->contains(fileDirPath)) {
        // Получаем список относительных директорий
        const auto dirParts = fileDirPath.right(fileDirPath.length() - projectDirPath.length())
                                  .split(QDir::separator(), Qt::SkipEmptyParts);
        assert(!dirParts.isEmpty());

        // Устанавливаем "корневой" указатель, относительно которого будут создаваться новые папки
        subDirItem = rootItem;
        // Устанавливаем "корневой" путь, к которому постепенно будут добавляться поддиректории
        auto relativeDirPath = projectDirPath;
        // Для надежности используем эту переменную, которая показывает, что неизвестная папка
        // была найдена, соответственно, в projectSubDirs не может быть найден указатель на элемент
        // папки
        bool unknownDirFound = false;
        for (const auto &dirPart : dirParts) {
            assert(subDirItem != nullptr);
            // Получаем путь к следующей папке
            relativeDirPath += QStringLiteral("%1%2").arg(QDir::separator()).arg(dirPart);
            if (!projectSubDirs->contains(relativeDirPath)) {
                // Если папка не найдена, то до "конца" dirParts создаем новые папки
                unknownDirFound = true;

                auto *newDir = initDirItem(dirPart, relativeDirPath, false);
                subDirItem->appendRow(newDir);
                subDirItem = newDir;
                (*projectSubDirs)[relativeDirPath] = subDirItem;
            }
            else {
                // Если папка найдена, то меняем "корневой" указатель на эту папку
                assert(!unknownDirFound);
                subDirItem = projectSubDirs->value(relativeDirPath, nullptr);
            }
        }
    }
    else {
        subDirItem = projectSubDirs->value(fileDirPath, nullptr);
    }
    assert(subDirItem != nullptr);
    // Добавляем к элементу папки новый файл
    subDirItem->appendRow(
        initFileItem(fileInfo.fileName(), fileInfo.absoluteFilePath(), isScriptsTree));
}

MainGui::MainGui(const QString &projectPath, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainGui)
{
    ui->setupUi(this);

    // Устанавливаем шрифты для логов
    QFontDatabase fontDatabase;
    auto font = QApplication::font();
    if (fontDatabase.hasFamily("Hack")) {
        font.setFamily("Hack");
    }
    ui->qtadaLogTextEdit->setFont(font);
    ui->appLogTextEdit->setFont(font);

    // По-умолчанию прячем все настройки, пока не откроем скрипт
    ui->recordAndSettingsWidget->setVisible(false);

    // Так как в Qt Designer нельзя добавить Spacer в QToolBar, то
    // делаем это вручную
    auto *toolSpacer = new QWidget(this);
    toolSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->toolBar->addWidget(toolSpacer);
    ui->toolBar->addAction(ui->actionRunCurrent);
    ui->toolBar->addAction(ui->actionRunAll);

    // Инициализация плейсхолдера, отображающегося при отсутствии открытых файлов
    fileNotOpenedLabel_ = new QLabel(this);
    fileNotOpenedLabel_->setAlignment(Qt::AlignCenter);
    fileNotOpenedLabel_->setText("No files are open.\n"
                                 "Select the file in the project tree on the left.");
    // Инициализация TabWidget, в котором будем отображать открытые файлы
    editorsTabWidget_ = new QTabWidget(this);
    editorsTabWidget_->setMovable(true);
    editorsTabWidget_->setTabsClosable(true);
    editorsTabWidget_->setDocumentMode(true);
    editorsTabWidget_->setVisible(false);
    // Инициализация макета под редактирование файлов
    auto *contentLayout = new QVBoxLayout(ui->contentWidget);
    contentLayout->addWidget(fileNotOpenedLabel_);
    contentLayout->addWidget(editorsTabWidget_);

    // Инициализация значений для выбора поведения при обращении к делегатам
    ui->textIndexBehaviorComboBox->addItem(QStringLiteral("Index"),
                                           static_cast<int>(TextIndexBehavior::OnlyIndex));
    ui->textIndexBehaviorComboBox->addItem(QStringLiteral("Text"),
                                           static_cast<int>(TextIndexBehavior::OnlyText));
    ui->textIndexBehaviorComboBox->addItem(QStringLiteral("Index & Text"),
                                           static_cast<int>(TextIndexBehavior::TextIndex));

    configureProject(projectPath);

    connect(ui->actionNewScript, &QAction::triggered, this,
            [this]() { addNewFileToProject(true, true); });
    connect(ui->actionNewSource, &QAction::triggered, this,
            [this]() { addNewFileToProject(true, false); });
    connect(ui->actionAddScript, &QAction::triggered, this,
            [this]() { addNewFileToProject(false, true); });
    connect(ui->actionAddSource, &QAction::triggered, this,
            [this]() { addNewFileToProject(false, false); });

    connect(ui->projectFilesView, &QTreeView::customContextMenuRequested, this,
            &MainGui::showProjectTreeContextMenu);
    connect(ui->projectFilesView, &QTreeView::doubleClicked, this, &MainGui::openFile);
    connect(editorsTabWidget_, &QTabWidget::tabCloseRequested, this, &MainGui::closeFileInEditor);
    connect(editorsTabWidget_, &QTabWidget::currentChanged, this,
            &MainGui::checkIfCurrentTabIsScript);

    connect(ui->lineIndexSpinBox, &QSpinBox::textChanged, this, &MainGui::handleSettingsChange);
    connect(ui->indentWidthSpinBox, &QSpinBox::textChanged, this, &MainGui::handleSettingsChange);
    connect(ui->blockCommentLinesSpinBox, &QSpinBox::textChanged, this,
            &MainGui::handleSettingsChange);
    connect(ui->duplicateMouseEventCheckBox, &QCheckBox::stateChanged, this,
            &MainGui::handleSettingsChange);
    connect(ui->textIndexBehaviorComboBox, &QComboBox::currentTextChanged, this,
            &MainGui::handleSettingsChange);
    connect(ui->needToGenerateCycleCheckBox, &QCheckBox::stateChanged, this,
            &MainGui::handleSettingsChange);
    connect(ui->cycleMinimumCountSpinBox, &QSpinBox::textChanged, this,
            &MainGui::handleSettingsChange);
    connect(ui->recordAppArgsEdit, &QLineEdit::editingFinished, this,
            &MainGui::handleSettingsChange);
    connect(ui->runAppArgsEdit, &QLineEdit::editingFinished, this, &MainGui::handleSettingsChange);

    connect(ui->workingDirectoryEdit, &QLineEdit::textChanged, this,
            &MainGui::handleLaunchSettingsChange);
    connect(ui->timeoutSpinBox, &QSpinBox::textChanged, this, &MainGui::handleLaunchSettingsChange);

    connect(ui->lineIndexFromScriptButton, &QPushButton::clicked, this, [this] {
        assert(lastScriptEditor_ != nullptr);
        const auto lineNumber = lastScriptEditor_->lastHighlitedLine();
        assert(lineNumber <= ui->lineIndexSpinBox->maximum());
        ui->lineIndexSpinBox->setValue(lineNumber);
    });

    connect(ui->actionCloseApplication, &QAction::triggered, qApp, &QApplication::exit);
    connect(ui->actionNewProject, &QAction::triggered, this, [this] {
        InitDialog startDialog;
        if (startDialog.exec() == QDialog::Accepted) {
            configureProject(startDialog.selectedProjectPath());
        }
    });

    connect(ui->recordButton, &QPushButton::clicked, this,
            [this] { startupScriptWriterLauncher(false); });
    connect(ui->updateButton, &QPushButton::clicked, this,
            [this] { startupScriptWriterLauncher(true); });

    connect(ui->actionRunAll, &QAction::triggered, this, &MainGui::runAllScripts);
    connect(ui->actionRunCurrent, &QAction::triggered, this, &MainGui::runCurrentScript);
}

MainGui::~MainGui()
{
    if (project_ != nullptr && saveProjectFileOnExit_) {
        saveGuiParamsToProjectFile();
        project_->deleteLater();
        project_ = nullptr;
    }

    delete ui;
}

bool MainGui::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate) {
        if (uiInitialized_) {
            updateProjectFileView(true);
        }
        else {
            // Сразу после .show() этого делать нельзя, так как
            // событие WindowActivate может возникнуть позже ->
            // будет "лишний" вызов функции
            uiInitialized_ = true;
        }
    }
    return QMainWindow::event(event);
}

void MainGui::closeEvent(QCloseEvent *event)
{
    if (!editorsTabWidget_->isVisible()) {
        event->accept();
        return;
    }
    for (int i = 0; i < editorsTabWidget_->count(); i++) {
        auto editor = qobject_cast<FileEditor *>(editorsTabWidget_->widget(i));
        assert(editor != nullptr);
        if (editor->isChanged()) {
            const auto confirm = QMessageBox::question(
                this, paths::QTADA_UNSAVED_CHANGES_HEADER,
                "You have unsaved changes. Are you sure you want to exit?\n",
                QMessageBox::No | QMessageBox::Yes, QMessageBox::No);
            if (confirm == QMessageBox::Yes) {
                event->accept();
            }
            else {
                event->ignore();
            }
            return;
        }
    }
    event->accept();
}

void MainGui::configureProject(const QString &projectPath) noexcept
{
    //! TODO: Нужно решить проблему с отображением в кириллице. По идее,
    //! достаточно использования toUtf8/fromUtf8

    // Если открываем новый прооект
    if (project_ != nullptr) {
        if (projectPath == project_->fileName()) {
            return;
        }

        saveGuiParamsToProjectFile();
        for (int i = 0; i < editorsTabWidget_->count(); i++) {
            closeFileInEditor(i);
        }
        project_->deleteLater();
    }
    project_ = new QSettings(projectPath, QSettings::IniFormat);

    updateProjectFileView(false);
    setGuiParamsFromProjectFile();

    project_->beginGroup(paths::PROJECT_LAUNCH_GROUP);
    LaunchSettings launchSettings;
    launchSettings.workingDirectory = project_->value(paths::PROJECT_WORKING_DIR, "").toString();
    launchSettings.timeoutValue
        = project_->value(paths::PROJECT_WORKING_DIR, MINIMUM_CYCLE_COUNT).toInt();
    project_->endGroup();
    updateCurrentLaunchSettings(std::move(launchSettings));
}

void MainGui::updateProjectFileView(bool isExternal) noexcept
{
    assert(project_ != nullptr);

    const auto projectFileInfo = QFileInfo(project_->fileName());
    if (!tools::isExistingFileAccessible(projectFileInfo)) {
        saveProjectFileOnExit_ = false;
        QMessageBox::critical(this, paths::QTADA_ERROR_HEADER,
                              QStringLiteral("The project file not accessible."));
        //! TODO: нужно придумать как "филиграннее" обрабатывать эту ситуацию
        QApplication::exit(1);
        return;
    }

    const auto appPath = project_->value(paths::PROJECT_APP_PATH, "").toString().trimmed();
    const auto appPathCheck = tools::checkProjectAppPath(appPath);
    if (appPathCheck != AppPathCheck::Ok) {
        saveProjectFileOnExit_ = false;
        QMessageBox::critical(this, paths::QTADA_ERROR_HEADER,
                              QStringLiteral("The testing application not accessible."));
        //! TODO: нужно придумать как "филиграннее" обрабатывать эту ситуацию
        QApplication::exit(1);
        return;
    }

    bool needToUpdateModel = false;
    const auto currentScripts = getAccessiblePaths(projectFileInfo, true);
    ui->actionRunAll->setEnabled(!currentScripts.isEmpty());
    if (currentScripts != lastScripts_) {
        needToUpdateModel = true;
        lastScripts_ = std::move(currentScripts);
    }

    const auto currentSources = getAccessiblePaths(projectFileInfo, false);
    if (currentSources != lastSources_) {
        needToUpdateModel = true;
        lastSources_ = std::move(currentSources);
    }

    if (!needToUpdateModel && uiInitialized_) {
        return;
    }

    if (isExternal) {
        QMessageBox::warning(this, paths::QTADA_WARNING_HEADER,
                             QStringLiteral("Project resources have been changed externally."));
    }

    auto *projectFilesModel = new QStandardItemModel(this);

    const auto projectDir = projectFileInfo.dir();
    auto *rootProjectDir
        = initDirItem(projectDir.dirName(), projectDir.absolutePath(), false, true);
    projectFilesModel->appendRow(rootProjectDir);

    rootProjectDir->appendRow(
        new CustomStandardItem(projectFileInfo.fileName(), projectFileInfo.absoluteFilePath(),
                               QIcon(":/icons/project.svg"), FileRole::ProjectRole));

    const auto projectDirPath = projectDir.absolutePath();

    auto *scriptsDirItem = initDirItem(QStringLiteral("Scripts"), QString(), true);
    configureSubTree(scriptsDirItem, projectDirPath, true);
    rootProjectDir->appendRow(scriptsDirItem);

    auto *sourceDirItem = initDirItem(QStringLiteral("Sources"), QString(), true);
    configureSubTree(sourceDirItem, projectDirPath, false);
    rootProjectDir->appendRow(sourceDirItem);

    const auto appFileInfo = QFileInfo(appPath);
    projectFilesModel->appendRow(
        new CustomStandardItem(appFileInfo.fileName(), appFileInfo.absoluteFilePath(),
                               QIcon(":/icons/test_app.svg"), FileRole::TestAppRole, false));

    tools::deleteModels(ui->projectFilesView);
    ui->projectFilesView->setModel(projectFilesModel);

    //! TODO: позже нужно "не трогать" раскрытие узлов дерева при добавлении
    //! новых файлов в проект внутри приложения
    // Раскрываем: <ProjectDir>, Scripts, Sources
    for (int row = 0; row < projectFilesModel->rowCount(); row++) {
        const auto index = projectFilesModel->index(row, 0);
        ui->projectFilesView->expand(index);

        for (int subRow = 0; subRow < projectFilesModel->itemFromIndex(index)->rowCount();
             subRow++) {
            ui->projectFilesView->expand(projectFilesModel->index(subRow, 0, index));
        }
    }

    //! TODO: нужно ли реагировать на "ручное" изменение настроек (RecordSettings, RunSettings)
    //! в файле проекта? По идее мы "постоянно" храним этим параметры внутри GUI в валидном виде,
    //! а если реагировать на "ручное" изменение, то на текущий момент это было бы очень ресурсо-
    //! затратно, так как каждый набор параметров нужно было бы проверять на валидность.
    //!
    //! Поэтому пока что все-таки было решено игнорировать такие изменения (также, как и игнорируем
    //! изменение настроек GUI).
}

QStringList MainGui::getAccessiblePaths(const QFileInfo &projectInfo, bool isScripts) noexcept
{
    assert(project_ != nullptr);

    // Считываем пути к файлам
    const auto rawFilesPaths
        = project_->value(isScripts ? paths::PROJECT_SCRIPTS : paths::PROJECT_SOURCES, {})
              .toStringList();
    if (rawFilesPaths.isEmpty()) {
        return QStringList();
    }

    // "Засовывая" rawFilesPaths в std::set мы одновременно избавляемся от дубликатов
    // и сортируем строки по алфавиту
    auto filesPaths = std::set<QString>(rawFilesPaths.begin(), rawFilesPaths.end());
    // Чтобы исключить дублирование файла проекта в дереве
    filesPaths.erase(projectInfo.absoluteFilePath());

    // Список файлов, которые "подходят" для нашего проекта
    QStringList acceptedFiles;
    // Список файлов, которые "не подходят" для нашего проекта, не существуют, не читаемы,
    // не доступны для записи или не являющиеся файлами
    QStringList discardFiles;

    const auto projectDirPath = projectInfo.dir().absolutePath();
    for (auto filePath : filesPaths) {
        filePath = filePath.trimmed();
        if (filePath.isEmpty()) {
            continue;
        }

        QFileInfo fileInfo(filePath);
        if (!tools::isExistingFileAccessible(fileInfo)
            || (isScripts && fileInfo.suffix() != "js")) {
            discardFiles.append(filePath);
            continue;
        }
        acceptedFiles.append(filePath);
    }

    project_->setValue(isScripts ? paths::PROJECT_SCRIPTS : paths::PROJECT_SOURCES,
                       acceptedFiles.isEmpty() ? QStringList("") : acceptedFiles);
    flushProjectFile();
    if (!discardFiles.isEmpty()) {
        QMessageBox::warning(
            this, paths::QTADA_WARNING_HEADER,
            QStringLiteral("These files are not applicable to the project, "
                           "so they have been removed from the project file:\n-- %1")
                .arg(discardFiles.join("\n-- ")));
    }

    return acceptedFiles;
}

void MainGui::configureSubTree(CustomStandardItem *rootItem, const QString &projectDirPath,
                               bool isScriptsTree) noexcept
{
    assert(rootItem != nullptr);
    // Файлы, которые лежат в корневой папке проекта. Выносим их отдельно, чтобы
    // добавить в конце инициализации, после всех подпапок
    QVector<QFileInfo> projectDirFilesInfo;
    // Файлы, которые не относятся к корневой папке проекта
    QVector<QFileInfo> otherPathsInfo;
    // Структура данных для папок корневой директории проекта, где:
    // {путь к папке} -> {указатель на элемент модели}
    auto projectSubDirs = std::make_unique<QMap<QString, CustomStandardItem *>>();

    for (const auto &filePath : (isScriptsTree ? lastScripts_ : lastSources_)) {
        QFileInfo fileInfo(filePath);

        const auto fileDirPath = fileInfo.dir().absolutePath();
        if (!fileInfo.absoluteFilePath().startsWith(projectDirPath + QDir::separator())) {
            // Если начало абсолютного пути к папке в которой лежит файл не соответствует
            // пути к корневой папке проекта, то файл лежит вне корневой папки проекта
            otherPathsInfo.push_back(fileInfo);
            continue;
        }

        // Проверка лежит ли файл в корневой папке проекта
        if (fileDirPath == projectDirPath) {
            projectDirFilesInfo.append(fileInfo);
            continue;
        }

        handleSubDirectories(projectDirPath, rootItem, isScriptsTree, projectSubDirs.get(),
                             fileInfo);
    }

    // Добавляем в конец rootItem файлы, лежащие в корневой папке проекта
    for (const auto &fileInfo : projectDirFilesInfo) {
        rootItem->appendRow(
            initFileItem(fileInfo.fileName(), fileInfo.absoluteFilePath(), isScriptsTree));
    }

    if (!otherPathsInfo.isEmpty()) {
        // Если были обнаружены файлы, лежащие вне корневой папки проекта, то в конец
        // добавляем папку, в которой будут расположены все "внешние" файлы
        projectSubDirs->clear();
        auto *otherPathsDirItem = initDirItem("<Other paths>", QString(), true);
        for (const auto &otherPathFileInfo : otherPathsInfo) {
            handleSubDirectories(QString(), otherPathsDirItem, isScriptsTree, projectSubDirs.get(),
                                 otherPathFileInfo);
        }
        rootItem->appendRow(otherPathsDirItem);
    }
}

void MainGui::saveGuiParamsToProjectFile() noexcept
{
    assert(project_ != nullptr);

    project_->beginGroup(paths::PROJECT_GUI_GROUP);

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

    project_->setValue(paths::PROJECT_TOOL_BAR_POSITION, this->toolBarArea(ui->toolBar));
    project_->setValue(paths::PROJECT_LINE_WRAP_MODE, ui->actionLineWrap->isChecked());

    project_->endGroup();
    flushProjectFile();
}

void MainGui::setGuiParamsFromProjectFile() noexcept
{
    assert(project_ != nullptr);

    bool toolBarPosIsOk = false;
    auto *toolBar = ui->toolBar;
    assert(toolBar != nullptr);

    project_->beginGroup(paths::PROJECT_GUI_GROUP);
    const auto toolBarPos
        = project_->value(paths::PROJECT_TOOL_BAR_POSITION, Qt::ToolBarArea::TopToolBarArea)
              .toInt(&toolBarPosIsOk);
    const auto lineWrapMode = project_->value(paths::PROJECT_LINE_WRAP_MODE, false).toBool();
    const auto contentProjectSizes = project_->value(paths::PROJECT_CONTENT_SIZES, {}).toList();
    const auto mainProjectSizes = project_->value(paths::PROJECT_MAIN_SIZES, {}).toList();
    project_->endGroup();

    if (toolBarPosIsOk && toolBarPos >= Qt::ToolBarArea::LeftToolBarArea
        && toolBarPos <= Qt::ToolBarArea::BottomToolBarArea) {
        this->addToolBar(static_cast<Qt::ToolBarArea>(toolBarPos), toolBar);
    }
    else {
        this->addToolBar(Qt::ToolBarArea::TopToolBarArea, toolBar);
    }

    ui->actionLineWrap->setChecked(lineWrapMode);

    if (contentProjectSizes.isEmpty() || contentProjectSizes.size() != 3) {
        ui->contentSplitter->setSizes({ ui->projectFilesWidget->minimumSizeHint().width(), -1,
                                        ui->recordAndSettingsWidget->minimumSizeHint().width() });
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

void MainGui::addNewFileToProject(bool isNewFileMode, bool isScript) noexcept
{
    assert(project_ != nullptr);
    const auto projectInfo = QFileInfo(project_->fileName());
    const auto projectPath = projectInfo.absoluteFilePath();

    const auto defaultDir = projectInfo.dir().absolutePath();
    const auto filter
        = isScript ? QStringLiteral("JavaScript (*.js)") : QStringLiteral("All files (*)");
    const auto path
        = (isNewFileMode
               ? QFileDialog::getSaveFileName(
                   this, isScript ? paths::QTADA_NEW_SCRIPT_HEADER : paths::QTADA_NEW_SOURCE_HEADER,
                   defaultDir, filter)
               : QFileDialog::getOpenFileName(
                   this, isScript ? paths::QTADA_ADD_SCRIPT_HEADER : paths::QTADA_ADD_SOURCE_HEADER,
                   defaultDir, filter))
              .trimmed();

    if (path.isEmpty()) {
        return;
    }

    const auto newFileInfo = QFileInfo(path);
    if (!isNewFileMode && !tools::isExistingFileAccessible(newFileInfo)) {
        QMessageBox::warning(
            this, paths::QTADA_WARNING_HEADER,
            QStringLiteral("The %1 file is not accessible.").arg(isScript ? "script" : "source"));
        return;
    }

    if (isScript && newFileInfo.suffix() != "js") {
        QMessageBox::warning(
            this, paths::QTADA_WARNING_HEADER,
            QStringLiteral("The script must be a JavaScript file with a .js extension."));
        return;
    }

    const auto newFilePath = newFileInfo.absoluteFilePath();
    if (projectPath == newFilePath) {
        QMessageBox::warning(this, paths::QTADA_WARNING_HEADER,
                             isNewFileMode
                                 ? QStringLiteral("You can't rewrite current project file.")
                                 : QStringLiteral("You can't add current project file as source."));
        return;
    }

    if (isNewFileMode) {
        auto newFile = QFile(newFilePath);
        if (!newFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(this, paths::QTADA_WARNING_HEADER,
                                 QStringLiteral("The %1 file can't be created.")
                                     .arg(isScript ? "script" : "source"));
            return;
        }
        newFile.close();
        assert(newFileInfo.exists());
    }

    auto paths = project_->value(isScript ? paths::PROJECT_SCRIPTS : paths::PROJECT_SOURCES, {})
                     .toStringList();
    paths.append(newFilePath);
    project_->setValue(isScript ? paths::PROJECT_SCRIPTS : paths::PROJECT_SOURCES, paths);
    flushProjectFile();
    updateProjectFileView(false);
}

void MainGui::showProjectTreeContextMenu(const QPoint &pos) noexcept
{
    const auto index = ui->projectFilesView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    auto *model = qobject_cast<QStandardItemModel *>(ui->projectFilesView->model());
    assert(model != nullptr);

    auto *rawItem = model->itemFromIndex(index);
    auto *item = static_cast<CustomStandardItem *>(rawItem);
    assert(item != nullptr);

    const auto role = item->role();

    if (role == FileRole::None) {
        return;
    }

    const auto path = item->data(role).value<QString>();
    assert(!path.isEmpty());

    QMenu contextMenu;
    const auto isScript = role == FileRole::ScriptRole;

    switch (role) {
    case FileRole::ScriptRole: {
        contextMenu.addAction(QStringLiteral("Run Test Script"), this,
                              [this, path] { runScript(path); });
        contextMenu.addSeparator();
    }
    case FileRole::SourceRole:
    case FileRole::ProjectRole: {
        contextMenu.addAction(QStringLiteral("Open in Editor"), this,
                              [this, index] { openFile(index); });
        if (role != FileRole::ProjectRole) {
            contextMenu.addAction(QStringLiteral("Remove from Project"), this,
                                  [this, path, isScript] { removeFromProject(path, isScript); });
        }
        contextMenu.addSeparator();
        contextMenu.addAction(QStringLiteral("Open Externally"), this,
                              [this, path] { openExternally(path); });
        break;
    }
    case FileRole::TestAppRole: {
        contextMenu.addAction(QStringLiteral("Execute"), this,
                              [this, path] { executeApplication(path); });
        contextMenu.addSeparator();
        break;
    }
    case FileRole::DirRole: {
        contextMenu.addAction(QStringLiteral("Remove From Project"), this,
                              [this, path] { removeDirFromProject(path); });
        contextMenu.addSeparator();
    }
    case FileRole::RootDirRole: {
        contextMenu.addAction(QStringLiteral("Open Folder"), this,
                              [this, path] { openFolder(path); });
        break;
    }
    default:
        Q_UNREACHABLE();
    }

    if (role != FileRole::DirRole && role != FileRole::RootDirRole) {
        contextMenu.addAction(QStringLiteral("Show in Folder"), this,
                              [this, path] { showInFolder(path); });
    }
    if (role != FileRole::ProjectRole && role != FileRole::TestAppRole
        && role != FileRole::RootDirRole) {
        contextMenu.addSeparator();
        contextMenu.addAction(QStringLiteral("Rename"), this,
                              [this, model, index] { renameFile(model, index); });
    }
    if (role == FileRole::ScriptRole || role == FileRole::SourceRole) {
        contextMenu.addAction(QStringLiteral("Delete"), this,
                              [this, path, isScript] { deleteFile(path, isScript); });
    }

    /*
     * Итого должны получить следующие меню:
     * 1) Для ScriptRole / SourceRole:
     *      - Run Test Script (только для ScriptRole)
     *      ------------------
     *      - Open in Editor
     *      - Remove from Project
     *      ------------------
     *      - Open Externally
     *      - Show in Folder
     *      ------------------
     *      - Rename
     *      - Delete
     *
     * 2) Для DirRole:
     *      - Remove from Project
     *      ------------------
     *      - Open Folder
     *      ------------------
     *      - Rename
     *      //! TODO: - Delete
     *
     * 3) Для ProjectRole:
     *      - Open in Editor
     *      ------------------
     *      - Open Externally
     *      - Show in Folder
     *
     * 4) Для RootDirRole:
     *      - Open Folder
     *
     * 4) Для TestAppRole:
     *      - Execute
     *      ------------------
     *      - Show in Folder
     */
    contextMenu.exec(ui->projectFilesView->viewport()->mapToGlobal(pos));
}

void MainGui::removeFromProject(const QString &path, bool isScript) noexcept
{
    for (int i = 0; i < editorsTabWidget_->count(); i++) {
        auto *editor = qobject_cast<FileEditor *>(editorsTabWidget_->widget(i));
        assert(editor != nullptr);
        if (path == editor->filePath()) {
            bool closeResult = editor->closeFile();
            if (!closeResult) {
                return;
            }
            closeFileInEditor(i);
            break;
        }
    }

    assert(project_ != nullptr);
    auto paths = project_->value(isScript ? paths::PROJECT_SCRIPTS : paths::PROJECT_SOURCES, {})
                     .toStringList();
    assert(paths.contains(path));
    paths.removeAll(path);
    if (paths.isEmpty()) {
        // Чтобы не было INVALID в файле проекта
        paths.push_back("");
    }
    project_->setValue(isScript ? paths::PROJECT_SCRIPTS : paths::PROJECT_SOURCES, paths);
    flushProjectFile();
    updateScriptPathForSettings(path);
    updateProjectFileView(false);
}

void MainGui::removeDirFromProject(const QString &path) noexcept
{
    assert(project_ != nullptr);
    auto scriptPaths = project_->value(paths::PROJECT_SCRIPTS, {}).toStringList();
    auto sourcePaths = project_->value(paths::PROJECT_SOURCES, {}).toStringList();
    assert(scriptPaths.size() + sourcePaths.size() > 0);

    const auto dirPath = path + QDir::separator();
    auto remove = [this, dirPath](QStringList &paths) {
        QMutableListIterator<QString> iterator(paths);
        while (iterator.hasNext()) {
            const auto path = iterator.next();
            if (path.startsWith(dirPath)) {
                for (int i = 0; i < editorsTabWidget_->count(); i++) {
                    auto *editor = qobject_cast<FileEditor *>(editorsTabWidget_->widget(i));
                    assert(editor != nullptr);
                    if (path == editor->filePath()) {
                        bool closeResult = editor->closeFile();
                        if (!closeResult) {
                            return false;
                        }
                        closeFileInEditor(i);
                        break;
                    }
                }
                iterator.remove();
            }
        }
        if (paths.isEmpty()) {
            // Чтобы не было INVALID в файле проекта
            paths.push_back("");
        }
        return true;
    };

    bool removeResult = remove(scriptPaths);
    if (!removeResult) {
        return;
    }
    removeResult = remove(sourcePaths);
    if (!removeResult) {
        return;
    }

    project_->setValue(paths::PROJECT_SCRIPTS, scriptPaths);
    project_->setValue(paths::PROJECT_SOURCES, sourcePaths);
    flushProjectFile();
    updateProjectFileView(false);
}

void MainGui::renameFile(QStandardItemModel *model, const QModelIndex &index) noexcept
{
    assert(model != nullptr);
    assert(index.isValid());

    auto *item = model->itemFromIndex(index);
    assert(item != nullptr);
    const auto oldName = item->data(Qt::DisplayRole).value<QString>();
    assert(!oldName.isEmpty());
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    auto *delegate = ui->projectFilesView->itemDelegate(index);
    assert(delegate != nullptr);
    connect(delegate, &QAbstractItemDelegate::closeEditor, this, [this, model, delegate, item] {
        disconnect(model, &QStandardItemModel::itemChanged, this, 0);
        disconnect(delegate, &QAbstractItemDelegate::closeEditor, this, 0);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    });

    connect(
        model, &QStandardItemModel::itemChanged, this,
        [this, model, oldName](QStandardItem *rawItem) { doRenameFile(model, rawItem, oldName); });

    ui->projectFilesView->edit(index);
}

void MainGui::deleteFile(const QString &path, bool isScript) noexcept
{
    QFile file(path);
    assert(file.exists());
    if (!file.remove()) {
        QMessageBox::critical(this, paths::QTADA_ERROR_HEADER,
                              QStringLiteral("File '%1' deletion failed.").arg(path));
        return;
    }

    for (int i = 0; i < editorsTabWidget_->count(); i++) {
        auto *editor = qobject_cast<FileEditor *>(editorsTabWidget_->widget(i));
        assert(editor != nullptr);
        if (editor->filePath() == path) {
            editor->closeFile(false);
            closeFileInEditor(i);
            break;
        }
    }

    assert(project_ != nullptr);
    auto paths = project_->value(isScript ? paths::PROJECT_SCRIPTS : paths::PROJECT_SOURCES, {})
                     .toStringList();
    assert(!paths.isEmpty());
    assert(paths.contains(path));
    paths.removeAll(path);
    project_->setValue(isScript ? paths::PROJECT_SCRIPTS : paths::PROJECT_SOURCES, paths);
    flushProjectFile();
    updateScriptPathForSettings(path);
    updateProjectFileView(false);
}

void MainGui::openExternally(const QString &path) noexcept
{
    QFileInfo fileInfo(path);
    assert(fileInfo.exists());
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainGui::showInFolder(const QString &path) noexcept
{
    QFileInfo fileInfo(path);
    assert(fileInfo.exists());
    //! TODO: нужно подсвечивать файл в директории
    openFolder(fileInfo.absolutePath());
}

void MainGui::openFolder(const QString &path) noexcept
{
    QFileInfo fileInfo(path);
    assert(fileInfo.exists());
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainGui::executeApplication(const QString &path) noexcept
{
    QFileInfo fileInfo(path);
    assert(fileInfo.exists());
    //! TODO: желательно дополнительно запрашивать у пользователя
    //! аргументы запуска
    QProcess::startDetached(path, {});
}

void MainGui::doRenameFile(QStandardItemModel *model, QStandardItem *rawItem,
                           const QString &oldName) noexcept
{
    disconnect(model, &QStandardItemModel::itemChanged, this, 0);

    auto *item = static_cast<CustomStandardItem *>(rawItem);
    assert(item != nullptr);
    const auto role = item->role();
    assert(role == FileRole::DirRole || role == FileRole::ScriptRole
           || role == FileRole::SourceRole);

    auto rawNewName = item->data(Qt::DisplayRole).value<QString>();
    const auto newName
        = rawNewName.right(rawNewName.length() - rawNewName.lastIndexOf(QDir::separator()) - 1);
    if (newName.isEmpty()) {
        item->setData(oldName, Qt::DisplayRole);
        return;
    }
    if (newName != rawNewName) {
        item->setData(newName, Qt::DisplayRole);
    }

    if (role == FileRole::ScriptRole
        && newName.right(newName.length() - newName.lastIndexOf('.') - 1) != "js") {
        item->setData(oldName, Qt::DisplayRole);
        QMessageBox::warning(
            this, paths::QTADA_WARNING_HEADER,
            QStringLiteral("The script must be a JavaScript file with a .js extension."));
        return;
    }

    const auto oldPath = item->data(role).value<QString>();
    const auto newPath = oldPath.left(oldPath.lastIndexOf(QDir::separator()) + 1) + newName;

    QFileInfo newFileInfo(newPath);
    if (newFileInfo.exists()) {
        item->setData(oldName, Qt::DisplayRole);
        QMessageBox::warning(this, paths::QTADA_WARNING_HEADER,
                             QStringLiteral("The %1 at '%2' already exists.")
                                 .arg(role == FileRole::DirRole ? "directory" : "file")
                                 .arg(newPath));
        return;
    }

    assert(project_ != nullptr);
    switch (role) {
    case FileRole::ScriptRole:
    case FileRole::SourceRole: {
        QFile oldFile(oldPath);
        assert(oldFile.exists());

        if (!oldFile.rename(newPath)) {
            item->setData(oldName, Qt::DisplayRole);
            QMessageBox::critical(
                this, paths::QTADA_ERROR_HEADER,
                QStringLiteral("Renaming '%1' -> '&2' failed.").arg(oldPath).arg(newPath));
            return;
        }

        auto filesPaths = project_
                              ->value(role == FileRole::ScriptRole ? paths::PROJECT_SCRIPTS
                                                                   : paths::PROJECT_SOURCES,
                                      {})
                              .toStringList();
        assert(!filesPaths.isEmpty());
        assert(filesPaths.contains(oldPath));
        filesPaths.removeAll(oldPath);
        filesPaths.append(newPath);
        project_->setValue(role == FileRole::ScriptRole ? paths::PROJECT_SCRIPTS
                                                        : paths::PROJECT_SOURCES,
                           filesPaths);
        flushProjectFile();

        for (int i = 0; i < editorsTabWidget_->count(); i++) {
            auto *editor = qobject_cast<FileEditor *>(editorsTabWidget_->widget(i));
            assert(editor != nullptr);
            if (editor->filePath() == oldPath) {
                editor->updateFilePath(newPath);
                break;
            }
        }

        if (role == FileRole::ScriptRole) {
            updateScriptPathForSettings(oldPath, newPath);
        }
        break;
    }
    case FileRole::DirRole: {
        QDir oldDir(oldPath);
        assert(oldDir.exists());

        if (!oldDir.rename(oldPath, newPath)) {
            item->setData(oldName, Qt::DisplayRole);
            QMessageBox::critical(
                this, paths::QTADA_ERROR_HEADER,
                QStringLiteral("Renaming '%1' -> '&2' failed.").arg(oldPath).arg(newPath));
            return;
        }

        const auto oldDirPath = oldPath + QDir::separator();
        const auto newDirPath = newPath + QDir::separator();
        auto renamePaths = [this, oldDirPath, newDirPath](QStringList &filesPaths) {
            for (auto &filePath : filesPaths) {
                if (filePath.startsWith(oldDirPath)) {
                    const auto oldFilePath = filePath;
                    filePath.replace(0, oldDirPath.length(), newDirPath);

                    for (int i = 0; i < editorsTabWidget_->count(); i++) {
                        auto *editor = qobject_cast<FileEditor *>(editorsTabWidget_->widget(i));
                        assert(editor != nullptr);
                        if (editor->filePath() == oldFilePath) {
                            editor->updateFilePath(filePath);
                            if (editor->role() == FileRole::ScriptRole) {
                                updateScriptPathForSettings(oldFilePath, filePath);
                            }
                            break;
                        }
                    }
                }
            }
        };

        auto scriptPaths = project_->value(paths::PROJECT_SCRIPTS, {}).toStringList();
        auto sourcePaths = project_->value(paths::PROJECT_SOURCES, {}).toStringList();
        assert(scriptPaths.size() + sourcePaths.size() > 0);

        renamePaths(scriptPaths);
        renamePaths(sourcePaths);

        project_->setValue(paths::PROJECT_SCRIPTS, scriptPaths);
        project_->setValue(paths::PROJECT_SOURCES, sourcePaths);
        flushProjectFile();
        break;
    }
    default:
        Q_UNREACHABLE();
    }
    updateProjectFileView(false);
}

void MainGui::openFile(const QModelIndex &index) noexcept
{
    assert(index.isValid());

    const auto *rawModel = ui->projectFilesView->model();
    const auto *model = qobject_cast<const QStandardItemModel *>(rawModel);
    assert(model != nullptr);

    const auto *rawItem = model->itemFromIndex(index);
    const auto *item = static_cast<const CustomStandardItem *>(rawItem);
    assert(item != nullptr);

    const auto role = item->role();
    if (role == FileRole::None) {
        return;
    }

    const auto path = item->data(role).value<QString>();
    assert(!path.isEmpty());

    if (role == FileRole::TestAppRole) {
        executeApplication(std::move(path));
        return;
    }

    if (role != FileRole::ProjectRole && role != FileRole::ScriptRole
        && role != FileRole::SourceRole) {
        return;
    }

    for (int i = 0; i < editorsTabWidget_->count(); i++) {
        const auto tabEditor = qobject_cast<const FileEditor *>(editorsTabWidget_->widget(i));
        assert(tabEditor != nullptr);
        if (tabEditor->filePath() == path) {
            if (editorsTabWidget_->currentIndex() != i) {
                editorsTabWidget_->setCurrentIndex(i);
            }
            return;
        }
    }

    const auto tabName = item->data(Qt::DisplayRole).value<QString>();
    assert(!tabName.isEmpty());
    const auto tabIcon = item->icon();
    assert(!tabIcon.isNull());

    auto *fileEditor = new FileEditor(path, role, editorsTabWidget_, ui->actionLineWrap);
    const auto isFileReadable = fileEditor->readFile();
    if (!isFileReadable) {
        fileEditor->deleteLater();
        return;
    }

    if (role == FileRole::ProjectRole) {
        connect(fileEditor, &FileEditor::projectFileHasChanged, this, [this] {
            project_->sync();
            updateProjectFileView(false);
        });
        connect(this, &MainGui::projectFileHasChanged, fileEditor, [this, fileEditor] {
            const auto readResult = fileEditor->reReadFile();
            if (!readResult) {
                disconnect(this, &MainGui::projectFileHasChanged, fileEditor, 0);
                closeFileInEditor(editorsTabWidget_->indexOf(fileEditor));
            }
        });
    }
    else if (role == FileRole::ScriptRole) {
        project_->beginGroup(paths::PROJECT_RECORD_GROUP);
        const auto recordJson = project_->value(path, QByteArray()).toByteArray();
        project_->endGroup();
        project_->beginGroup(paths::PROJECT_RUN_GROUP);
        const auto runJson = project_->value(path, QByteArray()).toByteArray();
        project_->endGroup();

        const auto recordSettings
            = recordJson.isEmpty() ? RecordSettings() : RecordSettings::fromJson(recordJson, true);
        const auto runSettings
            = recordJson.isEmpty() ? RunSettings() : RunSettings::fromJson(runJson, true);

        //! TODO: Очень некрасивое решение. Когда мы загоняем настройки в элементы GUI, то
        //! благодаря их ограничениям, мы точно получим валидные значения. Но в текущей версии
        //! получится так, что сначала мы тут установливаем новые значения в настройки, а потом,
        //! при изменении текущего индекса QTabWidget, функция установки "новых" значений вызовется
        //! повторно.
        updateCurrentSettings({ std::move(recordSettings), std::move(runSettings) });
        const auto settings = readCurrentSettings();
        fileEditor->setSettings(settings);
        saveScriptSettings(path, std::move(settings));
    }

    const auto tabIndex = editorsTabWidget_->addTab(fileEditor, tabIcon, tabName);
    editorsTabWidget_->setCurrentIndex(tabIndex);

    if (editorsTabWidget_->isVisible()) {
        return;
    }

    assert(fileNotOpenedLabel_->isVisible());
    fileNotOpenedLabel_->setVisible(false);
    editorsTabWidget_->setVisible(true);
}

void MainGui::closeFileInEditor(int index) noexcept
{
    auto *fileEditor = qobject_cast<FileEditor *>(editorsTabWidget_->widget(index));
    assert(fileEditor != nullptr);
    const auto closeResult = fileEditor->closeFile();
    if (!closeResult) {
        return;
    }
    editorsTabWidget_->removeTab(index);
    if (fileEditor == lastScriptEditor_) {
        lastScriptEditor_ = nullptr;
    }
    fileEditor->deleteLater();

    if (editorsTabWidget_->count() == 0) {
        editorsTabWidget_->setVisible(false);
        ui->recordAndSettingsWidget->setVisible(false);
        fileNotOpenedLabel_->setVisible(true);
    }
}

void MainGui::checkIfCurrentTabIsScript(int index) noexcept
{
    if (index == -1) {
        return;
    }
    auto *editor = qobject_cast<FileEditor *>(editorsTabWidget_->widget(index));
    assert(editor != nullptr);
    const auto isScript = editor->role() == FileRole::ScriptRole;
    ui->recordAndSettingsWidget->setVisible(isScript);

    if (lastScriptEditor_ != nullptr) {
        disconnect(lastScriptEditor_, &FileEditor::lineCountChanged, this, 0);
        lastScriptEditor_ = nullptr;
    }

    ui->actionRunCurrent->setEnabled(isScript);
    if (!isScript) {
        return;
    }

    updateCurrentSettings(editor->getSettings());
    ui->lineIndexSpinBox->setMaximum(editor->lineCount());

    lastScriptEditor_ = editor;
    connect(lastScriptEditor_, &FileEditor::lineCountChanged, this,
            [this](int lineCount) { ui->lineIndexSpinBox->setMaximum(lineCount); });
}

MainGui::Settings MainGui::readCurrentSettings() const noexcept
{
    RecordSettings recordSettings;
    recordSettings.indentWidth = ui->indentWidthSpinBox->value();
    recordSettings.blockCommentMinimumCount = ui->blockCommentLinesSpinBox->value();
    recordSettings.duplicateMouseEvent = ui->duplicateMouseEventCheckBox->isChecked();
    recordSettings.textIndexBehavior
        = static_cast<TextIndexBehavior>(ui->textIndexBehaviorComboBox->currentData().toInt());
    recordSettings.needToGenerateCycle = ui->needToGenerateCycleCheckBox->isChecked();
    recordSettings.cycleMinimumCount = ui->cycleMinimumCountSpinBox->value();
    recordSettings.appendLineIndex = ui->lineIndexSpinBox->value();
    recordSettings.executeArgs = ui->recordAppArgsEdit->text();

    RunSettings runSettings;
    runSettings.executeArgs = ui->runAppArgsEdit->text();
    runSettings.attempsNumber = ui->attempsNumberSpinBox->value();
    runSettings.retryInterval = ui->retryIntervalSpinBox->value();

    return { recordSettings, runSettings };
}

void MainGui::saveScriptSettings(const QString &path, ConstSettings settings) noexcept
{
    assert(project_ != nullptr);
    project_->beginGroup(paths::PROJECT_RECORD_GROUP);
    project_->setValue(path, settings.first.toJson(true));
    project_->endGroup();
    project_->beginGroup(paths::PROJECT_RUN_GROUP);
    project_->setValue(path, settings.second.toJson(true));
    project_->endGroup();
    flushProjectFile();
}

void MainGui::handleSettingsChange() noexcept
{
    ui->updateButton->setEnabled(ui->lineIndexSpinBox->value() > 0);
    ui->cycleMinimumCountWidget->setEnabled(ui->needToGenerateCycleCheckBox->isChecked());
    if (settingsChangeHandlerBlocked_) {
        return;
    }

    auto *editor = qobject_cast<FileEditor *>(editorsTabWidget_->currentWidget());
    assert(editor != nullptr);
    assert(editor->role() == FileRole::ScriptRole);
    const auto currentScriptPath = editor->filePath();
    assert(!currentScriptPath.isEmpty());

    const auto settings = readCurrentSettings();
    saveScriptSettings(currentScriptPath, settings);

    // Делаем это только для того, чтобы получать и устанавливать настройки
    // при изменении текущей вкладки удобнее и быстрее, чем чтение из файла проекта.
    editor->setSettings(std::move(settings));
}

void MainGui::handleLaunchSettingsChange() noexcept
{
    if (launchSettingsChangeHandlerBlocked_) {
        return;
    }
    assert(project_ != nullptr);
    project_->beginGroup(paths::PROJECT_LAUNCH_GROUP);
    project_->setValue(paths::PROJECT_WORKING_DIR, ui->workingDirectoryEdit->text());
    project_->setValue(paths::PROJECT_TIMEOUT, ui->timeoutSpinBox->value());
    project_->endGroup();
    flushProjectFile();
}

void MainGui::updateCurrentSettings(ConstSettings settings) noexcept
{
    settingsChangeHandlerBlocked_ = true;

    const auto recordSettings = settings.first;
    const auto runSettings = settings.second;

    ui->indentWidthSpinBox->setValue(recordSettings.indentWidth);
    ui->blockCommentLinesSpinBox->setValue(recordSettings.blockCommentMinimumCount);
    ui->duplicateMouseEventCheckBox->setChecked(recordSettings.duplicateMouseEvent);
    bool foundItem = false;
    const auto textIndexBehavior = static_cast<int>(recordSettings.textIndexBehavior);
    for (int i = 0; i < ui->textIndexBehaviorComboBox->count(); i++) {
        if (ui->textIndexBehaviorComboBox->itemData(i) == textIndexBehavior) {
            foundItem = true;
            ui->textIndexBehaviorComboBox->setCurrentIndex(i);
            break;
        }
    }
    assert(foundItem == true);
    ui->needToGenerateCycleCheckBox->setChecked(recordSettings.needToGenerateCycle);
    ui->cycleMinimumCountSpinBox->setValue(recordSettings.cycleMinimumCount);
    ui->lineIndexSpinBox->setValue(recordSettings.appendLineIndex);
    ui->recordAppArgsEdit->setText(recordSettings.executeArgs);

    ui->runAppArgsEdit->setText(runSettings.executeArgs);
    ui->attempsNumberSpinBox->setValue(runSettings.attempsNumber);
    ui->retryIntervalSpinBox->setValue(runSettings.retryInterval);

    settingsChangeHandlerBlocked_ = false;
}

void MainGui::updateCurrentLaunchSettings(const LaunchSettings &settings) noexcept
{
    launchSettingsChangeHandlerBlocked_ = true;
    ui->workingDirectoryEdit->setText(settings.workingDirectory);
    ui->timeoutSpinBox->setValue(settings.timeoutValue);
    launchSettingsChangeHandlerBlocked_ = false;
}

void MainGui::updateScriptPathForSettings(const QString &oldPath, const QString &newPath) noexcept
{
    assert(!oldPath.isEmpty());

    auto updatePath = [this, oldPath, newPath](bool isRecordMode) {
        project_->beginGroup(isRecordMode ? paths::PROJECT_RECORD_GROUP : paths::PROJECT_RUN_GROUP);
        if (project_->contains(oldPath)) {
            const auto settings = project_->value(oldPath, QByteArray()).toByteArray();
            project_->remove(oldPath);
            if (!newPath.isEmpty()) {
                project_->setValue(newPath, settings);
            }
        }
        project_->endGroup();
    };

    updatePath(true);
    updatePath(false);
    flushProjectFile();
}

void MainGui::flushProjectFile() noexcept
{
    assert(project_ != nullptr);
    project_->sync();
    emit projectFileHasChanged();
}

launcher::UserLaunchOptions MainGui::getUserOptionsForLauncher(const QString &appPath,
                                                               const QString &scriptPath,
                                                               LaunchType type,
                                                               bool isUpdateMode) const noexcept
{
    assert(project_ != nullptr);

    launcher::UserLaunchOptions options;
    options.type = type;
    options.timeoutValue = ui->timeoutSpinBox->value();

    const auto workingDirectory = ui->workingDirectoryEdit->text().trimmed();
    if (workingDirectory.isEmpty()) {
        const auto projectDirPath = QFileInfo(project_->fileName()).absolutePath();
        assert(!projectDirPath.isEmpty());
        options.workingDirectory = std::move(projectDirPath);
    }
    else {
        options.workingDirectory = std::move(workingDirectory);
    }

    QString executeArgs;
    switch (type) {
    case LaunchType::Record: {
        assert(lastScriptEditor_ != nullptr);
        auto recordSettings = lastScriptEditor_->getRecordSettings();
        recordSettings.scriptPath = scriptPath;
        recordSettings.scriptWriteMode
            = isUpdateMode ? ScriptWriteMode::UpdateScript : ScriptWriteMode::NewScript;
        options.recordSettings = recordSettings;
        executeArgs = recordSettings.executeArgs;
        break;
    }
    case LaunchType::Run: {
        RunSettings runSettings;
        project_->beginGroup(paths::PROJECT_RUN_GROUP);
        const auto rawRunSettings = project_->value(scriptPath, "").toByteArray();
        project_->endGroup();
        if (!rawRunSettings.isEmpty()) {
            auto tmpRunSettings = RunSettings::fromJson(std::move(rawRunSettings), true);
            if (tmpRunSettings.isValid()) {
                runSettings = std::move(tmpRunSettings);
            }
        }
        runSettings.scriptPath = scriptPath;
        options.runSettings = runSettings;
        executeArgs = runSettings.executeArgs;
        break;
    }
    default:
        Q_UNREACHABLE();
    }

    auto launchAppArguments = executeArgs.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
    launchAppArguments.push_front(std::move(appPath));
    options.launchAppArguments = std::move(launchAppArguments);

    return options;
}

launcher::UserLaunchOptions MainGui::getUserOptionsForRecord(const QString &appPath,
                                                             const QString &scriptPath,
                                                             bool isUpdateMode) const noexcept
{
    return getUserOptionsForLauncher(appPath, scriptPath, LaunchType::Record, isUpdateMode);
}

launcher::UserLaunchOptions MainGui::getUserOptionsForRun(const QString &appPath,
                                                          const QString &scriptPath) const noexcept
{
    return getUserOptionsForLauncher(appPath, scriptPath, LaunchType::Run, false);
}

void MainGui::startupScriptWriterLauncher(bool isUpdateMode) noexcept
{
    using namespace launcher;
    assert(launcher_ == nullptr);
    assert(lastScriptEditor_ != nullptr);

    if (!isUpdateMode && !lastScriptEditor_->toPlainText().isEmpty()) {
        const auto confirm = QMessageBox::question(
            this, paths::QTADA_OVERWRITE_SCRIPT_HEADER,
            QStringLiteral("It seems that the current script is not empty. The selected mode will "
                           "completely overwrite the current script and you will lose your current "
                           "data.\nAre you sure you want to continue?"),
            QMessageBox::No | QMessageBox::Yes, QMessageBox::No);
        if (confirm == QMessageBox::No) {
            return;
        }
    }

    //! TODO: вообще, у нас путь к тестируемому приложению хранится в дереве проекта,
    //! и может все-таки лучше этот путь доставать оттуда? Причем мы точно знаем индекс
    //! этого элемента в дереве: (1, 0)
    const auto appPath = project_->value(paths::PROJECT_APP_PATH, "").toString();
    assert(!appPath.isEmpty());
    const auto scriptPath = lastScriptEditor_->filePath();
    assert(!scriptPath.isEmpty());
    prepareLogTextEditsForRecording(appPath, scriptPath, true);

    launcher_ = new Launcher(getUserOptionsForRecord(appPath, scriptPath, isUpdateMode), true);
    connect(launcher_, &Launcher::launcherErrMessage, this, &MainGui::writeQtAdaErrMessage);
    connect(launcher_, &Launcher::launcherOutMessage, this, &MainGui::writeQtAdaOutMessage);
    connect(launcher_, &Launcher::stdMessage, this, &MainGui::writeAppOutMessage);
    connect(launcher_, &Launcher::launcherFinished, this, [this, appPath, scriptPath] {
        prepareLogTextEditsForRecording(appPath, scriptPath, false, launcher_->exitCode());

        const auto readResult = lastScriptEditor_->reReadFile();
        if (!readResult) {
            const auto editorIndex = editorsTabWidget_->indexOf(lastScriptEditor_);
            assert(editorIndex >= 0);
            closeFileInEditor(editorIndex);
        }
        launcher_->deleteLater();
        launcher_ = nullptr;

        this->setEnabled(true);
        this->showMaximized();
    });

    if (launcher_->launch()) {
        lastScriptEditor_->saveFile();
        this->setEnabled(false);
        this->showMinimized();
    }
    else {
        QMessageBox::critical(this, paths::QTADA_ERROR_HEADER,
                              QStringLiteral("An error occurred while launching the application. "
                                             "For more information view log messages."));
    }
}

void MainGui::prepareTextEditForAppLog(const QString &timestamp, const QString &appPath,
                                       bool isStarting, int exitCode) noexcept
{
    if (isStarting) {
        const auto appLogText
            = ui->appLogTextEdit->toPlainText().trimmed().toHtmlEscaped().replace("\n", "<br>");
        if (!appLogText.isEmpty()) {
            writeColoredMessage(false, appLogText, GUI_INACTIVE_LOG_COLOR, false);
        }
        writeColoredMessage(false, QStringLiteral("%1: Starting %2...").arg(timestamp).arg(appPath),
                            GUI_SERVICE_LOG_COLOR);
    }
    else {
        writeColoredMessage(
            false,
            QStringLiteral("%1: %2 exited with code %3").arg(timestamp).arg(appPath).arg(exitCode),
            GUI_SERVICE_LOG_COLOR);
    }
}

void MainGui::prepareLogTextEditsForRecording(const QString &appPath, const QString &scriptPath,
                                              bool isStarting, int exitCode) noexcept
{
    const auto timestamp = QTime::currentTime().toString("hh:mm:ss");
    prepareTextEditForAppLog(timestamp, appPath, isStarting, exitCode);
    if (isStarting) {
        const auto qtadaLogText
            = ui->qtadaLogTextEdit->toPlainText().trimmed().toHtmlEscaped().replace("\n", "<br>");
        if (!qtadaLogText.isEmpty()) {
            writeColoredMessage(true, qtadaLogText, GUI_INACTIVE_LOG_COLOR, false);
        }
        writeColoredMessage(
            true, QStringLiteral("%1: Starting recording %2...").arg(timestamp).arg(scriptPath),
            GUI_SERVICE_LOG_COLOR);
    }
    else {
        writeColoredMessage(
            true,
            QStringLiteral("%1: %2 %3")
                .arg(timestamp)
                .arg(scriptPath)
                .arg(exitCode != 0 ? "wasn't written due to an error" : "recorded successfully"),
            GUI_SERVICE_LOG_COLOR);
    }
}

void MainGui::prepareLogTextEditsForRunning(bool isStarting) noexcept
{
    const auto timestamp = QTime::currentTime().toString("hh:mm:ss");
    if (isStarting) {
        const auto qtadaLogText
            = ui->qtadaLogTextEdit->toPlainText().trimmed().toHtmlEscaped().replace("\n", "<br>");
        if (!qtadaLogText.isEmpty()) {
            writeColoredMessage(true, qtadaLogText, GUI_INACTIVE_LOG_COLOR, false);
        }
        writeColoredMessage(true, QStringLiteral("%1: Starting testing...").arg(timestamp),
                            GUI_SERVICE_LOG_COLOR);
    }
    else {
        writeColoredMessage(true, QStringLiteral("%1: Testing finished").arg(timestamp),
                            GUI_SERVICE_LOG_COLOR);
    }
}

void MainGui::runScript(const QString &path) noexcept
{
    assert(!path.isEmpty());
    startupScriptRunnerLauncher(QStringList(path));
}

void MainGui::runCurrentScript() noexcept
{
    assert(lastScriptEditor_ != nullptr);
    startupScriptRunnerLauncher(QStringList(lastScriptEditor_->filePath()));
}

void MainGui::runAllScripts() noexcept
{
    startupScriptRunnerLauncher(project_->value(paths::PROJECT_SCRIPTS, {}).toStringList());
}

void MainGui::startupScriptRunnerLauncher(QStringList scripts) noexcept
{
    using namespace launcher;
    assert(launcher_ == nullptr);

    scriptsRunData_.scripts = std::move(scripts);
    assert(!scriptsRunData_.scripts.isEmpty());
    const auto appPath = project_->value(paths::PROJECT_APP_PATH, "").toString();
    assert(!appPath.isEmpty());

    launcher_ = new Launcher(LaunchType::Run, true);
    connect(launcher_, &Launcher::launcherErrMessage, this, &MainGui::writeQtAdaErrMessage);
    connect(launcher_, &Launcher::launcherOutMessage, this, &MainGui::writeQtAdaOutMessage);
    connect(launcher_, &Launcher::scriptRunError, this, &MainGui::writeQtAdaErrMessage);
    connect(launcher_, &Launcher::scriptRunLog, this, &MainGui::writeQtAdaOutMessage);

    auto launcherRunner = [this, appPath] {
        assert(!scriptsRunData_.scripts.isEmpty());
        assert(launcher_ != nullptr);
        scriptsRunData_.currentScript = scriptsRunData_.scripts.takeFirst();
        writeScriptServiceMessage(
            QStringLiteral("[ Run      ] %1").arg(scriptsRunData_.currentScript));
        launcher_->updateLaunchOptions(
            getUserOptionsForRun(appPath, scriptsRunData_.currentScript));
        if (!launcher_->launch()) {
            return false;
        }
        return true;
    };

    connect(launcher_, &Launcher::launcherFinished, this, [this, launcherRunner] {
        if (launcher_->exitCode() == 0) {
            scriptsRunData_.testsPassed++;
            writeScriptServiceMessage(
                QStringLiteral("[       OK ] %1").arg(launcher_->currentScriptPath()));
        }
        else {
            scriptsRunData_.testsFailed++;
            writeScriptServiceMessage(
                QStringLiteral("[    ERROR ] %1").arg(launcher_->currentScriptPath()));
        }

        if (scriptsRunData_.scripts.isEmpty()) {
            if (scriptsRunData_.testsFailed > 0) {
                writeScriptServiceMessage(QStringLiteral("[  FAILED  ] Passed: %1 | Failed: %2")
                                              .arg(scriptsRunData_.testsPassed)
                                              .arg(scriptsRunData_.testsFailed),
                                          true);
            }
            else {
                writeScriptServiceMessage(
                    QStringLiteral("[  PASSED  ] %1 tests").arg(scriptsRunData_.testsPassed), true);
            }

            scriptsRunData_.clear();
            prepareLogTextEditsForRunning(false);
            this->setEnabled(true);
            launcher_->deleteLater();
            launcher_ = nullptr;
        }
        else {
            launcherRunner();
        }
    });

    prepareLogTextEditsForRunning(true);
    ui->tabWidget->setCurrentIndex(0);
    this->setEnabled(false);
    if (!launcherRunner()) {
        QMessageBox::critical(this, paths::QTADA_ERROR_HEADER,
                              QStringLiteral("An error occurred while launching the application. "
                                             "For more information view log messages."));
    }
}

void MainGui::writeColoredMessage(bool isForQtAdaLog, const QString &msg, const QString &color,
                                  bool isAppendMode) noexcept
{
    const auto coloredMessage
        = QStringLiteral("<span style=\"white-space: pre-wrap;%1\">%2</span>")
              .arg(color.isEmpty() ? "" : QStringLiteral(" color: %1;").arg(color))
              .arg(msg.trimmed());
    if (isForQtAdaLog) {
        if (isAppendMode) {
            ui->qtadaLogTextEdit->append(std::move(coloredMessage));
        }
        else {
            ui->qtadaLogTextEdit->setHtml(std::move(coloredMessage));
            ui->qtadaLogTextEdit->append("");
        }
    }
    else {
        if (isAppendMode) {
            ui->appLogTextEdit->append(std::move(coloredMessage));
        }
        else {
            ui->appLogTextEdit->setHtml(std::move(coloredMessage));
            ui->appLogTextEdit->append("");
        }
    }
}

void MainGui::writeQtAdaErrMessage(const QString &msg) noexcept
{
    writeColoredMessage(true, msg, GUI_ERROR_LOG_COLOR);
}

void MainGui::writeQtAdaOutMessage(const QString &msg) noexcept
{
    writeColoredMessage(true, msg);
}

void MainGui::writeScriptServiceMessage(const QString &msg, bool isResult) noexcept
{
    writeColoredMessage(true, msg, isResult ? GUI_SCRIPT_RESULT_COLOR : GUI_SCRIPT_SERVICE_COLOR);
}

void MainGui::writeAppOutMessage(const QString &msg) noexcept
{
    writeColoredMessage(false, msg);
}
} // namespace QtAda::gui
