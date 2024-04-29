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

    // Объявление компонентов для макета под раздел "недавние проекты"
    QLabel *recentLabel = new QLabel("Recent Projects:", this);
    QListView *recentView = nullptr;
    QLabel *noRecentProjectsLabel = nullptr;
    // Инициализация компонентов для раздела "недавние проекты"
    auto *recentModel = initRecentModel();
    if (recentModel != nullptr) {
        recentView = new QListView(this);
        recentView->setModel(recentModel);
        recentView->setIconSize(QSize(30, 30));
        recentView->setSpacing(5);
        recentView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
    else {
        noRecentProjectsLabel = new QLabel("No recent projects", this);
        noRecentProjectsLabel->setStyleSheet("QLabel { color : #F8961E ; }");
    }
    // Инициализация макета под раздел "недавние проекты"
    auto *recentWidget = new QWidget(this);
    auto *recentLayout = new QVBoxLayout(recentWidget);
    recentLayout->addWidget(recentLabel);
    if (recentView != nullptr) {
        recentLayout->addWidget(recentView);
    }
    else if (noRecentProjectsLabel != nullptr) {
        recentLayout->addWidget(noRecentProjectsLabel);
        tools::setVSpacer(recentLayout);
    }
    else {
        Q_UNREACHABLE();
    }

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

QStandardItemModel *StartDialog::initRecentModel() noexcept
{
    auto recentProjects = config_->value(paths::INI_RECENT_PROJECTS).toStringList();
    QStringList confirmedProjectPaths;
    QStringList confirmedProjectNames;
    while (!recentProjects.isEmpty()) {
        const auto projectPath = recentProjects.takeFirst().trimmed();
        if (projectPath.isEmpty()) {
            continue;
        }
        const auto projectFile = QFileInfo(projectPath);
        if (!projectFile.exists() || !projectFile.isReadable() || !projectFile.isWritable()
            || projectFile.suffix() != QStringLiteral("qtada")) {
            continue;
        }
        confirmedProjectNames.push_back(projectFile.fileName());
        confirmedProjectPaths.push_back(std::move(projectPath));
    }

    if (confirmedProjectPaths.isEmpty()) {
        config_->setValue(paths::INI_RECENT_PROJECTS, "");
        return nullptr;
    }
    config_->setValue(paths::INI_RECENT_PROJECTS, confirmedProjectPaths);

    assert(confirmedProjectPaths.size() == confirmedProjectNames.size());
    auto *recentModel_ = new QStandardItemModel(this);
    for (int i = 0; i < confirmedProjectPaths.size(); i++) {
        const auto recentInfo = QStringLiteral("%1\n%2")
                                    .arg(confirmedProjectNames.at(i))
                                    .arg(confirmedProjectPaths.at(i));
        auto *recentItem = new QStandardItem(std::move(recentInfo));
        recentItem->setIcon(QIcon(":/icons/project.svg"));
        recentModel_->appendRow(recentItem);
    }
    assert(recentModel_->rowCount() > 0);

    return recentModel_;
}

void StartDialog::handleNewProject() noexcept
{
}

void StartDialog::handleOpenProject() noexcept
{
}

void StartDialog::handleRecentProject() noexcept
{
}
} // namespace QtAda::gui
