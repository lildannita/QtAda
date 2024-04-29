#include "StartDialog.hpp"

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

#include "Paths.hpp"
#include "GuiTools.hpp"

namespace QtAda::gui {
StartDialog::StartDialog(QWidget *parent)
    : QDialog(parent)
    , config_{ new QSettings(paths::QTADA_CONFIG, QSettings::IniFormat) }
{
    assert(config_ != nullptr);

    // Инициализация параметров окна
    this->setWindowTitle("QtAda | Init Project");
    this->setWindowIcon(QIcon(":/icons/app.png"));

    // Инициализация основных кнопок
    auto *openProjectButton = initButton("Open Project...", ":/icons/open_project.svg");
    auto *newProjectButton = initButton("New Project...", ":/icons/new_project.svg");
    auto *closeButton = initButton("Close", ":/icons/close.svg");
    connect(newProjectButton, &QPushButton::clicked, this, &StartDialog::handleNewProject);
    connect(openProjectButton, &QPushButton::clicked, this, &StartDialog::handleOpenProject);
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
    QLabel *recentLabel = new QLabel("Recent Projects:", this);
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
    connect(recentView_, &QListView::clicked, this, &StartDialog::handleRecentProject);
    // Инициализация Label с сообщением об отсутсвии недавних проектов
    QLabel *emptyRecentLabel = new QLabel("No recent projects", this);
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

    // Инициализация основного макета
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(buttonsWidget);
    mainLayout->addWidget(tools::initSeparator(this));
    mainLayout->addWidget(recentWidget);

    // Настройка первоначальных размеров диалога
    const auto screenGeometry = QApplication::screenAt(QCursor::pos())->geometry();
    const auto prefferedSize = QSize(screenGeometry.width() * 0.3, screenGeometry.height() * 0.3);
    this->setMinimumSize(prefferedSize);
    this->resize(prefferedSize);
}

StartDialog::~StartDialog()
{
}

QPushButton *StartDialog::initButton(const QString &text, const QString &iconPath) noexcept
{
    auto *button = new QPushButton(this);
    button->setText(text);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(30, 30));
    button->setFocusPolicy(Qt::NoFocus);
    button->setStyleSheet("text-align:left;");
    button->setFlat(true);
    return button;
}

bool StartDialog::checkProjectFilePath(const QString &path, bool isOpenMode,
                                       bool needToShowMsg) noexcept
{
    assert(!path.isEmpty());
    auto fileInfo = QFileInfo(path);

    if (fileInfo.suffix() != paths::PROJECT_SUFFIX) {
        if (needToShowMsg) {
            QMessageBox::critical(this, "QtAda | Error",
                                  QStringLiteral("The project file extension must be '%1'.")
                                      .arg(paths::PROJECT_SUFFIX));
        }
        return false;
    }

    if (isOpenMode) {
        if (!fileInfo.exists()) {
            if (needToShowMsg) {
                QMessageBox::critical(this, "QtAda | Error",
                                      QStringLiteral("The project file doesn't exist."));
            }
            return false;
        }
        if (!(fileInfo.isFile() && fileInfo.isReadable() && fileInfo.isWritable())) {
            if (needToShowMsg) {
                QMessageBox::critical(this, "QtAda | Error",
                                      QStringLiteral("The project can't be opened."));
            }
            return false;
        }
    }
    else {
        auto file = QFile(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::critical(this, "QtAda | Error",
                                  QStringLiteral("The project file can't be created."));
            return false;
        }
        file.close();
    }

    return true;
}

void StartDialog::updateRecentModel() noexcept
{
    assert(recentModel_ != nullptr);
    recentModel_->clear();

    auto recentProjects = config_->value(paths::CONFIG_RECENT_PROJECTS).toStringList();
    QStringList confirmedProjectPaths;
    QStringList confirmedProjectNames;
    while (!recentProjects.isEmpty()) {
        const auto projectPath = recentProjects.takeFirst().trimmed();
        if (projectPath.isEmpty() || !checkProjectFilePath(projectPath, true, false)) {
            continue;
        }
        auto fileName = QFileInfo(projectPath).fileName();
        fileName = fileName.left(fileName.lastIndexOf('.'));
        confirmedProjectNames.push_back(std::move(fileName));
        confirmedProjectPaths.push_back(std::move(projectPath));
    }

    if (confirmedProjectPaths.isEmpty()) {
        config_->setValue(paths::CONFIG_RECENT_PROJECTS, "");
        return;
    }
    config_->setValue(paths::CONFIG_RECENT_PROJECTS, confirmedProjectPaths);

    assert(confirmedProjectPaths.size() == confirmedProjectNames.size());
    for (int i = 0; i < confirmedProjectPaths.size(); i++) {
        const auto recentPath = confirmedProjectPaths.at(i);
        const auto recentInfo
            = QStringLiteral("%1\n%2").arg(confirmedProjectNames.at(i)).arg(recentPath);
        auto *recentItem = new QStandardItem(recentInfo);
        recentItem->setIcon(QIcon(":/icons/project.svg"));
        recentItem->setData(std::move(recentPath), Qt::UserRole);
        recentModel_->appendRow(recentItem);
    }
    assert(recentModel_->rowCount() > 0);
}

void StartDialog::updateRecentInConfig(const QString &path) noexcept
{
    auto recentProjects = config_->value(paths::CONFIG_RECENT_PROJECTS).toStringList();
    if (recentProjects.contains(path)) {
        recentProjects.removeAll(path);
    }
    recentProjects.prepend(path);
    config_->setValue(paths::CONFIG_RECENT_PROJECTS, recentProjects);
}

void StartDialog::handleNewProject() noexcept
{
    const auto projectPath
        = QFileDialog::getSaveFileName(
              this, "QtAda | New Project",
              QStringLiteral("%1/untitled.%2").arg(QDir::homePath()).arg(paths::PROJECT_SUFFIX),
              QStringLiteral("QtAda (*.%1)").arg(paths::PROJECT_SUFFIX))
              .trimmed();
    if (projectPath.isEmpty() || !checkProjectFilePath(projectPath)) {
        return;
    }
    updateRecentInConfig(projectPath);

    //! TODO: открывать MainWindow
}

void StartDialog::handleOpenProject() noexcept
{
    const auto projectPath
        = QFileDialog::getOpenFileName(this, "QtAda | Open Project", QDir::homePath(),
                                       QStringLiteral("QtAda (*.%1)").arg(paths::PROJECT_SUFFIX))
              .trimmed();
    if (projectPath.isEmpty() || !checkProjectFilePath(projectPath, true)) {
        return;
    }
    updateRecentInConfig(projectPath);

    //! TODO: открывать MainWindow
}

void StartDialog::handleRecentProject(const QModelIndex &index) noexcept
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

    //! TODO: открывать MainWindow
}
} // namespace QtAda::gui
