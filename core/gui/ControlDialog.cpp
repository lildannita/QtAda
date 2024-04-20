#include "ControlDialog.hpp"

#include <QCoreApplication>
#include <QApplication>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>

namespace QtAda::core::gui {
ControlDialog::ControlDialog(bool closeWindowsOnExit, QWidget *parent) noexcept
    : QDialog{ parent }
    , closeWindowsOnExit_{ closeWindowsOnExit }
    , completeScriptButton_{ new QToolButton }
    , addVerificationButton_{ new QToolButton }
    , addCommentButton_{ new QToolButton }
    , pauseButton_{ new QToolButton }
    , playButton_{ new QToolButton }
    , cancelScriptButton_{ new QToolButton }
    , scriptWidget_{ new QWidget }
    , scriptLineLabel_{ new QLabel }
    , commentWidget_{ new QWidget }
    , commentTextEdit_{ new QTextEdit }
    , acceptCommentButton_{ new QPushButton }
    , clearCommentButton_{ new QPushButton }
// , verificationWidget_{ new QWidget }
{
    this->setWindowTitle("QtAda | Control Bar");
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    // Инициализация основных кнопок
    initToolButton(completeScriptButton_, "Complete Script", ":/icons/scenario_ready.svg");
    initToolButton(addVerificationButton_, "Add Verification", ":/icons/add_verification.svg");
    initToolButton(addCommentButton_, "Add Comment", ":/icons/add_comment.svg");
    initToolButton(pauseButton_, "Pause", ":/icons/pause.svg");
    initToolButton(playButton_, "Play", ":/icons/play.svg");
    initToolButton(cancelScriptButton_, "Cancel script", ":/icons/scenario_cancel.svg");
    addVerificationButton_->setCheckable(true);
    addCommentButton_->setCheckable(true);
    playButton_->setVisible(false);
    // Подключение слотов к основным кнопкам
    connect(completeScriptButton_, &QToolButton::clicked, this, &ControlDialog::completeScript);
    connect(addVerificationButton_, &QToolButton::clicked, this, &ControlDialog::addVerification);
    connect(addCommentButton_, &QToolButton::clicked, this, &ControlDialog::addComment);
    connect(pauseButton_, &QToolButton::clicked, this, &ControlDialog::pause);
    connect(playButton_, &QToolButton::clicked, this, &ControlDialog::play);
    connect(cancelScriptButton_, &QToolButton::clicked, this, &ControlDialog::cancelScript);
    // Инициализация макета с кнопками
    QWidget *buttons = new QWidget;
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttons);
    buttonLayout->addWidget(completeScriptButton_);
    buttonLayout->addWidget(addVerificationButton_);
    buttonLayout->addWidget(addCommentButton_);
    buttonLayout->addWidget(generateSeparator(false));
    buttonLayout->addWidget(pauseButton_);
    buttonLayout->addWidget(playButton_);
    buttonLayout->addWidget(cancelScriptButton_);

    // Инициализация строки, отображающей последнюю сгенерированную команду
    scriptLineLabel_->setTextFormat(Qt::PlainText);
    scriptLineLabel_->setWordWrap(true);
    scriptLineLabel_->setText("Start interacting in the application under test!");
    // Инициализация макета для строки
    QVBoxLayout *scriptLayout = new QVBoxLayout(scriptWidget_);
    scriptLayout->addWidget(generateSeparator());
    scriptLayout->addWidget(scriptLineLabel_);

    // Инициализация текстового редактора для ввода комментария
    commentTextEdit_->setPlaceholderText("Enter comment text...");
    // Инициализация кнопок для управления комментарием
    acceptCommentButton_->setText("Accept");
    acceptCommentButton_->setFocusPolicy(Qt::NoFocus);
    clearCommentButton_->setText("Clear");
    clearCommentButton_->setFocusPolicy(Qt::NoFocus);
    connect(acceptCommentButton_, &QPushButton::clicked, this, &ControlDialog::acceptComment);
    connect(clearCommentButton_, &QPushButton::clicked, this, &ControlDialog::clearComment);
    // Инициализация макета под кнопки для управления комментарием
    QWidget *commentButtons = new QWidget;
    QVBoxLayout *commentButtonsLayout = new QVBoxLayout(commentButtons);
    commentButtonsLayout->addWidget(acceptCommentButton_);
    commentButtonsLayout->addWidget(clearCommentButton_);
    // Инициализация макета для ввода комментариев
    QHBoxLayout *commentLayout = new QHBoxLayout(commentWidget_);
    commentLayout->addWidget(commentTextEdit_);
    commentLayout->addWidget(commentButtons);
    commentWidget_->setVisible(false);

    // Инициализация основного макета
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(buttons);
    mainLayout->addWidget(commentWidget_);
    mainLayout->addWidget(scriptWidget_);

    this->adjustSize();
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void ControlDialog::initToolButton(QToolButton *button, const QString &text,
                                   const QString &iconPath) noexcept
{
    assert(button != nullptr);
    button->setText(text);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(30, 30));
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setFocusPolicy(Qt::NoFocus);
}

void ControlDialog::completeScript() noexcept
{
    if (closeWindowsOnExit_) {
        QApplication::closeAllWindows();
    }
    QCoreApplication::quit();
}

void ControlDialog::addVerification() noexcept
{
    addCommentButton_->setChecked(false);
    handleVisibility();
}

void ControlDialog::addComment() noexcept
{
    addVerificationButton_->setChecked(false);
    handleVisibility();
}

void ControlDialog::pause() noexcept
{
    emit applicationPaused(true);
    pauseButton_->setVisible(false);
    playButton_->setVisible(true);
}

void ControlDialog::play() noexcept
{
    emit applicationPaused(false);
    pauseButton_->setVisible(true);
    playButton_->setVisible(false);
}

void ControlDialog::cancelScript() noexcept
{
    emit scriptCancelled();
    QCoreApplication::quit();
}

void ControlDialog::handleVisibility() noexcept
{
    // verificationWidget_->setVisible(addVerificationButton_->isChecked());
    commentWidget_->setVisible(addCommentButton_->isChecked());
}

void ControlDialog::acceptComment() noexcept
{
    emit newCommentLine(commentTextEdit_->toPlainText());
    clearComment();
}

void ControlDialog::clearComment() noexcept
{
    commentTextEdit_->clear();
}

QFrame *ControlDialog::generateSeparator(bool isHorizontal)
{
    QFrame *separator = new QFrame(this);
    separator->setFrameShape(isHorizontal ? QFrame::HLine : QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    return separator;
}
} // namespace QtAda::core::gui
