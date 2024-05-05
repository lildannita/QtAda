#include "InitDialog.hpp"

#include <QApplication>
#include <QScreen>
#include <QSettings>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QString>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QTextEdit>

#include "Paths.hpp"

namespace QtAda::gui {
InitDialog::InitDialog(QWidget *parent) noexcept
    : QDialog(parent)
    , config_{ new QSettings(paths::QTADA_CONFIG, QSettings::IniFormat) }
{
    assert(config_ != nullptr);

    // Инициализация параметров окна
    this->setWindowTitle(paths::QTADA_INIT_PROJECT_HEADER);
    this->setWindowIcon(QIcon(":/icons/app.png"));

    // Инициализация основных кнопок
    auto *openProjectButton = initButton("Open Project...", ":/icons/open_project.svg");
    auto *newProjectButton = initButton("New Project...", ":/icons/new_project.svg");
    auto *closeButton = initButton("Close", ":/icons/close.svg");
    connect(newProjectButton, &QPushButton::clicked, this, &InitDialog::handleNewProject);
    connect(openProjectButton, &QPushButton::clicked, this, &InitDialog::handleOpenProject);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    // Инициализация макета под основные кнопки
    auto *buttonsWidget = new QWidget(this);
    auto *buttonsLayout = new QVBoxLayout(buttonsWidget);
    buttonsLayout->addWidget(newProjectButton);
    buttonsLayout->addWidget(openProjectButton);
    tools::setVSpacer(buttonsLayout);
    buttonsLayout->addWidget(closeButton);
    buttonsWidget->setFixedWidth(buttonsLayout->sizeHint().width());

    // Инициализация компонентов для макета под раздел "недавние проекты"
    auto *recentLabel = new QLabel("Recent Projects:", this);
    // Инициализация ListView с недавними проектами
    recentModel_ = new QStandardItemModel(this);
    updateRecentModel();
    const auto isViewVisible = recentModel_->rowCount() > 0;
    recentView_ = new QListView(this);
    recentView_->setModel(recentModel_);
    recentView_->setIconSize(QSize(30, 30));
    recentView_->setSpacing(5);
    recentView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    recentView_->setSelectionMode(QListView::NoSelection);
    recentView_->setVisible(isViewVisible);
    connect(recentView_, &QListView::clicked, this, &InitDialog::handleRecentProject);
    // Инициализация Label с сообщением об отсутсвии недавних проектов
    auto *emptyRecentLabel = new QLabel("No recent projects", this);
    emptyRecentLabel->setStyleSheet("QLabel { color : #F8961E ; }");
    // Инициализация макета под Label с сообщением об отсутсвии недавних проектов
    emptyRecentWidget_ = new QWidget(this);
    auto *emptyRecentLayout = new QVBoxLayout(emptyRecentWidget_);
    emptyRecentLayout->addWidget(emptyRecentLabel);
    tools::setVSpacer(emptyRecentLayout);
    emptyRecentWidget_->setVisible(!isViewVisible);
    // Инициализация макета под раздел "недавние проекты"
    auto *recentWidget = new QWidget(this);
    auto *recentLayout = new QVBoxLayout(recentWidget);
    recentLayout->addWidget(recentLabel);
    recentLayout->addWidget(recentView_);
    recentLayout->addWidget(emptyRecentWidget_);

    // Инициализация макета под инициализацию проекта
    initWidget_ = new QWidget(this);
    auto *initLayout = new QHBoxLayout(initWidget_);
    initLayout->addWidget(buttonsWidget);
    initLayout->addWidget(tools::initSeparator(this));
    initLayout->addWidget(recentWidget);

    // Инициализация компонентов для выбора пути к исполняемому файлу
    appPathEdit_ = new QLineEdit(this);
    auto *searchButton = initButton("Search...", ":/icons/search.svg");
    connect(searchButton, &QPushButton::clicked, this, &InitDialog::handleSelectAppPath);
    connect(appPathEdit_, &QLineEdit::textChanged, this, &InitDialog::handleAppPathChanged);
    // Инициализация макета под выбор пути к исполняемому файлу
    auto *setPathWidget = new QWidget(this);
    auto *setPathLayout = new QHBoxLayout(setPathWidget);
    setPathLayout->addWidget(appPathEdit_);
    setPathLayout->addWidget(tools::initSeparator(this));
    setPathLayout->addWidget(searchButton);

    // Инициализация кнопок для подтверждения выбранного пути
    auto *backButton = initButton("Back", ":/icons/back.svg");
    confirmButton_ = initButton("Confirm", ":/icons/confirm.svg");
    confirmButton_->setEnabled(false);
    connect(backButton, &QPushButton::clicked, this, &InitDialog::handleBackToInit);
    connect(confirmButton_, &QPushButton::clicked, this, &InitDialog::handleConfirmAppPath);
    // Инициализация макета под кнопки для подтверждения выбранного пути
    auto *confirmWidget = new QWidget(this);
    auto *confirmLayout = new QHBoxLayout(confirmWidget);
    tools::setHSpacer(confirmLayout);
    confirmLayout->addWidget(backButton);
    confirmLayout->addWidget(confirmButton_);

    // Инициализация компонента под информацию о выбранном пути
    appInfoEdit_ = new QTextEdit(this);
    appInfoEdit_->setReadOnly(true);
    appInfoEdit_->setStyleSheet("background-color: transparent;");

    // Установка стандартных значений для текстовых элементов
    setDefaultInfoForAppPathWidget();

    // Инициализация Label с информацией об обновлении пути
    pathLabel_ = new QLabel(this);

    // Инициализация макета под настройку пути к исполняемому файлу
    appPathWidget_ = new QWidget(this);
    auto *appPathLayout = new QVBoxLayout(appPathWidget_);
    appPathLayout->addWidget(pathLabel_);
    appPathLayout->addWidget(setPathWidget);
    appPathLayout->addWidget(appInfoEdit_);
    appPathLayout->addWidget(confirmWidget);
    appPathWidget_->setVisible(false);

    // Инициализация основного макета
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(initWidget_);
    mainLayout->addWidget(appPathWidget_);

    // Настройка первоначальных размеров диалога
    const auto screenGeometry = QApplication::screenAt(QCursor::pos())->geometry();
    const auto prefferedSize = QSize(screenGeometry.width() * 0.3, screenGeometry.height() * 0.3);
    this->setMinimumSize(prefferedSize);
    this->resize(prefferedSize);
}

QPushButton *InitDialog::initButton(const QString &text, const QString &iconPath) noexcept
{
    auto *button = new QPushButton(this);
    button->setText(text);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(30, 30));
    button->setFocusPolicy(Qt::NoFocus);
    button->setStyleSheet("text-align:left;margin-left: 5px;");
    button->setFlat(true);
    return button;
}

bool InitDialog::checkProjectFilePath(const QString &path, bool isOpenMode,
                                      bool needToShowMsg) noexcept
{
    assert(!path.isEmpty());
    auto fileInfo = QFileInfo(path);

    if (fileInfo.suffix() != paths::PROJECT_SUFFIX) {
        if (needToShowMsg) {
            QMessageBox::critical(this, paths::QTADA_ERROR_HEADER,
                                  QStringLiteral("The project file extension must be '%1'.")
                                      .arg(paths::PROJECT_SUFFIX));
        }
        return false;
    }

    if (isOpenMode) {
        if (!fileInfo.exists()) {
            if (needToShowMsg) {
                QMessageBox::critical(this, paths::QTADA_ERROR_HEADER,
                                      QStringLiteral("The project file doesn't exist."));
            }
            return false;
        }
        if (!tools::isFileAccessible(fileInfo)) {
            if (needToShowMsg) {
                QMessageBox::critical(this, paths::QTADA_ERROR_HEADER,
                                      QStringLiteral("The project can't be opened."));
            }
            return false;
        }
    }
    else {
        auto file = QFile(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::critical(this, paths::QTADA_ERROR_HEADER,
                                  QStringLiteral("The project file can't be created."));
            return false;
        }
        file.close();
    }

    return true;
}

void InitDialog::updateRecentModel() noexcept
{
    assert(recentModel_ != nullptr);
    recentModel_->clear();

    auto recentProjects = config_->value(paths::CONFIG_RECENT_PROJECTS).toStringList();
    QStringList confirmedProjectPaths;
    while (!recentProjects.isEmpty()) {
        const auto projectPath = recentProjects.takeFirst().trimmed();
        if (projectPath.isEmpty() || !checkProjectFilePath(projectPath, true, false)) {
            continue;
        }
        confirmedProjectPaths.push_back(std::move(projectPath));
    }

    if (confirmedProjectPaths.isEmpty()) {
        config_->setValue(paths::CONFIG_RECENT_PROJECTS, "");
        return;
    }
    config_->setValue(paths::CONFIG_RECENT_PROJECTS, confirmedProjectPaths);

    for (const auto &path : confirmedProjectPaths) {
        const auto recentInfo
            = QStringLiteral("%1\n%2").arg(tools::fileNameWithoutSuffix(path)).arg(path);
        auto *recentItem = new QStandardItem(recentInfo);
        recentItem->setIcon(QIcon(":/icons/project.svg"));
        recentItem->setData(std::move(path), Qt::UserRole);
        recentModel_->appendRow(recentItem);
    }
    assert(recentModel_->rowCount() > 0);
}

void InitDialog::updateRecentInConfig(const QString &path) noexcept
{
    auto recentProjects = config_->value(paths::CONFIG_RECENT_PROJECTS).toStringList();
    if (recentProjects.contains(path)) {
        recentProjects.removeAll(path);
    }
    recentProjects.prepend(path);
    config_->setValue(paths::CONFIG_RECENT_PROJECTS, recentProjects);
}

void InitDialog::handleNewProject() noexcept
{
    const auto projectPath
        = QFileDialog::getSaveFileName(
              this, paths::QTADA_NEW_PROJECT_HEADER,
              QStringLiteral("%1/untitled.%2").arg(QDir::homePath()).arg(paths::PROJECT_SUFFIX),
              QStringLiteral("QtAda (*.%1)").arg(paths::PROJECT_SUFFIX))
              .trimmed();
    if (projectPath.isEmpty() || !checkProjectFilePath(projectPath)) {
        return;
    }
    updateRecentInConfig(projectPath);
    selectedProjectPath_ = std::move(projectPath);

    pathLabel_->setText("Set the path to the application you are going to test.");
    initWidget_->setVisible(false);
    appPathWidget_->setVisible(true);
}

void InitDialog::handleOpenProject() noexcept
{
    const auto projectPath
        = QFileDialog::getOpenFileName(this, paths::QTADA_OPEN_PROJECT_HEADER, QDir::homePath(),
                                       QStringLiteral("QtAda (*.%1)").arg(paths::PROJECT_SUFFIX))
              .trimmed();
    if (projectPath.isEmpty() || !checkProjectFilePath(projectPath, true)) {
        return;
    }
    updateRecentInConfig(projectPath);
    tryToAcceptPath(std::move(projectPath));
}

void InitDialog::handleRecentProject(const QModelIndex &index) noexcept
{
    assert(index.isValid());
    const auto projectPath = index.data(Qt::UserRole).value<QString>();
    assert(!projectPath.isEmpty());
    if (!checkProjectFilePath(projectPath, true)) {
        updateRecentModel();
        // Не пишем "в две строки", так как важно сначала выключить видимость
        // одного виджета, а потом включить видимость другого, чтобы диалог
        // не "прыгал".
        if (recentModel_->rowCount() > 0) {
            emptyRecentWidget_->setVisible(false);
            recentView_->setVisible(true);
        }
        else {
            recentView_->setVisible(false);
            emptyRecentWidget_->setVisible(true);
        }
        return;
    }
    updateRecentInConfig(projectPath);
    tryToAcceptPath(std::move(projectPath));
}

void InitDialog::tryToAcceptPath(const QString &path) noexcept
{
    selectedProjectPath_ = path;
    if (checkProjectFile()) {
        this->accept();
    }
    else {
        pathLabel_->setText("Update the path to the application you are going to test.");
        initWidget_->setVisible(false);
        appPathWidget_->setVisible(true);
    }
}

bool InitDialog::checkProjectFile() noexcept
{
    assert(!selectedProjectPath_.isEmpty());

    auto project = QSettings(selectedProjectPath_, QSettings::IniFormat);

    const auto appPath = project.value(paths::PROJECT_APP_PATH, "").toString().trimmed();
    switch (tools::checkProjectAppPath(appPath)) {
    case AppPathCheck::NoExecutable: {
        QMessageBox::warning(
            this, paths::QTADA_PROJECT_ERROR_HEADER,
            QStringLiteral("Invalid path to the tested application in the specified project file "
                           "(file does not exist or is not executable)."));
        return false;
    }
    case AppPathCheck::NoProbe: {
        QMessageBox::warning(
            this, paths::QTADA_PROJECT_ERROR_HEADER,
            QStringLiteral(
                "Can't find Probe ABI for the executable file at the specified path in the "
                "project file (the tested application should be built with Qt 5.15)."));
        return false;
    }
    case AppPathCheck::Ok: {
        return true;
    }
    }
    Q_UNREACHABLE();
}

void InitDialog::setDefaultInfoForAppPathWidget() noexcept
{
    appPathEdit_->clear();
    appInfoEdit_->setText("No path specified.");
}

void InitDialog::updateAppPathInfo(const AppPathCheck type) noexcept
{
    appInfoEdit_->clear();
    auto isError = type == AppPathCheck::NoExecutable;
    appInfoEdit_->setHtml(QStringLiteral("Executable: <font color=\"%1\">%2</font>")
                              .arg(isError ? "#F94144" : "#43AA8B")
                              .arg(isError ? "ERROR" : "OK"));
    isError = isError || type == AppPathCheck::NoProbe;
    appInfoEdit_->append(QStringLiteral("Probe ABI: <font color=\"%1\">%2</font>")
                             .arg(isError ? "#F94144" : "#43AA8B")
                             .arg(isError ? "ERROR" : "OK"));
}

void InitDialog::handleSelectAppPath() noexcept
{
    const auto appPath
        = QFileDialog::getOpenFileName(this, paths::QTADA_SELECT_EXE_HEADER, QDir::homePath())
              .trimmed();
    if (appPath.isEmpty()) {
        setDefaultInfoForAppPathWidget();
        return;
    }
    appPathEdit_->setText(appPath);

    const auto appPathType = tools::checkProjectAppPath(appPath);
    updateAppPathInfo(appPathType);
    confirmButton_->setEnabled(appPathType == AppPathCheck::Ok);
}

void InitDialog::handleBackToInit() noexcept
{
    selectedProjectPath_.clear();
    appPathWidget_->setVisible(false);
    initWidget_->setVisible(true);
    setDefaultInfoForAppPathWidget();
}

void InitDialog::handleConfirmAppPath() noexcept
{
    auto project = QSettings(selectedProjectPath_, QSettings::IniFormat);
    project.setValue(paths::PROJECT_APP_PATH, appPathEdit_->text().trimmed());
    this->accept();
}

void InitDialog::handleAppPathChanged() noexcept
{
    const auto appPath = appPathEdit_->text().trimmed();
    const auto appPathType = tools::checkProjectAppPath(appPath);
    updateAppPathInfo(appPathType);
    confirmButton_->setEnabled(appPathType == AppPathCheck::Ok);
}
} // namespace QtAda::gui
