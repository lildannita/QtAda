#include "StartDialog.hpp"

#include <QApplication>
#include <QScreen>
#include <QFont>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QString>

#include <QFileDialog>

#include <QScrollArea>
#include <QScrollBar>

#include "GuiTools.hpp"

namespace QtAda::gui {
StartDialog::StartDialog(QWidget *parent)
    : QDialog(parent)
{
    this->setWindowTitle("QtAda | Init Project");
    this->setWindowIcon(QIcon(":/icons/app.png"));

    auto *openProjectButton = initButton("Open Project...", ":/icons/open_project.svg");
    auto *newProjectButton = initButton("New Project...", ":/icons/new_project.svg");
    auto *closeButton = initButton("Close", ":/icons/close.svg");
    connect(newProjectButton, &QPushButton::clicked, this, &StartDialog::handleNewProject);
    connect(openProjectButton, &QPushButton::clicked, this, &StartDialog::handleOpenProject);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    auto *buttonsWidget = new QWidget(this);
    auto *buttonsLayout = new QVBoxLayout(buttonsWidget);
    buttonsLayout->addWidget(newProjectButton);
    buttonsLayout->addWidget(openProjectButton);
    buttonsLayout->addSpacerItem(
        new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
    buttonsLayout->addWidget(closeButton);
    buttonsWidget->setFixedWidth(buttonsLayout->sizeHint().width());

    auto *recentModel = new QStandardItemModel(this);
    for (int i = 0; i < 50; i++) {
        recentModel->appendRow(initRecentItem("A\nB"));
    }
    auto *recentView = new QListView(this);
    recentView->setModel(recentModel);
    recentView->setIconSize(QSize(30, 30));
    recentView->setSpacing(5);
    recentView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QLabel *recentLabel = new QLabel("Recent Projects:", this);
    auto font = recentLabel->font();

    auto *recentWidget = new QWidget(this);
    auto *recentLayout = new QVBoxLayout(recentWidget);
    recentLayout->addWidget(recentLabel);
    recentLayout->addWidget(recentView);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(buttonsWidget);
    mainLayout->addWidget(tools::initSeparator(this));
    mainLayout->addWidget(recentWidget);

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

QStandardItem *StartDialog::initRecentItem(const QString &text) noexcept
{
    auto *item = new QStandardItem(text);
    item->setIcon(QIcon(":/icons/project.svg"));
    return item;
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
