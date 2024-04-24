#include "InprocessDialog.hpp"

#include <QCoreApplication>
#include <QApplication>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>

//#include "utils/Tools.hpp"
#include "InprocessTools.hpp"
#include "PropertiesWatcher.hpp"

namespace QtAda::inprocess {
InprocessDialog::InprocessDialog(QWidget *parent) noexcept
    : QDialog{ parent }
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
    , inprocessController_ { new InprocessController }
    , propertiesWatcher_{ new PropertiesWatcher(inprocessController_, this) }
{
    // Настройка параметров окна диалога
    this->setWindowTitle("QtAda | Control Bar");
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    this->setWindowIcon(QIcon(":/icons/app.png"));

    // Инициализация основных кнопок
    initToolButton(completeScriptButton_, "Complete Script", ":/icons/scenario_ready.svg");
    initToolButton(addVerificationButton_, "Add Verification", ":/icons/add_verification.svg");
    initToolButton(addCommentButton_, "Add Comment", ":/icons/add_comment.svg");
    initToolButton(pauseButton_, "Pause", ":/icons/pause.svg");
    initToolButton(playButton_, "Play", ":/icons/play.svg");
    initToolButton(cancelScriptButton_, "Cancel script", ":/icons/scenario_cancel.svg");
    addVerificationButton_->setCheckable(true);
    addCommentButton_->setCheckable(true);
    playButton_->setFixedSize(pauseButton_->sizeHint());
    playButton_->setVisible(false);
    // Подключение слотов к основным кнопкам
    connect(completeScriptButton_, &QToolButton::clicked, this, &InprocessDialog::scriptCompleted);
    connect(addVerificationButton_, &QToolButton::clicked, this, &InprocessDialog::addVerification);
    connect(addCommentButton_, &QToolButton::clicked, this, &InprocessDialog::addComment);
    connect(pauseButton_, &QToolButton::clicked, this, &InprocessDialog::pause);
    connect(playButton_, &QToolButton::clicked, this, &InprocessDialog::play);
    connect(cancelScriptButton_, &QToolButton::clicked, this, &InprocessDialog::scriptCancelled);
    // Инициализация макета с кнопками
    QWidget *buttons = new QWidget;
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttons);
    buttonLayout->addWidget(completeScriptButton_);
    buttonLayout->addWidget(addVerificationButton_);
    buttonLayout->addWidget(addCommentButton_);
    buttonLayout->addWidget(generateSeparator(this));
    buttonLayout->addWidget(pauseButton_);
    buttonLayout->addWidget(playButton_);
    buttonLayout->addWidget(cancelScriptButton_);

    // Инициализация строки, отображающей последнюю сгенерированную команду
    scriptLineLabel_->setTextFormat(Qt::PlainText);
    scriptLineLabel_->setWordWrap(true);
    setTextToScriptLabel("Start interacting in the application under test!");
    // Инициализация макета для строки
    QVBoxLayout *scriptLayout = new QVBoxLayout(scriptWidget_);
    scriptLayout->addWidget(generateSeparator(this, true));
    scriptLayout->addWidget(scriptLineLabel_);

    // Инициализация текстового редактора для ввода комментария
    commentTextEdit_->setPlaceholderText("Enter comment text...");
    // Инициализация кнопок для управления комментарием
    acceptCommentButton_->setText("Accept");
    acceptCommentButton_->setFocusPolicy(Qt::NoFocus);
    clearCommentButton_->setText("Clear");
    clearCommentButton_->setFocusPolicy(Qt::NoFocus);
    connect(acceptCommentButton_, &QPushButton::clicked, this, &InprocessDialog::acceptComment);
    connect(clearCommentButton_, &QPushButton::clicked, this, &InprocessDialog::clearComment);
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
    mainLayout->addWidget(propertiesWatcher_);
    mainLayout->addWidget(commentWidget_);
    mainLayout->addWidget(scriptWidget_);

    this->adjustSize();
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);
}

InprocessDialog::~InprocessDialog() noexcept
{
    if (inprocessController_ != nullptr) {
        inprocessController_->deleteLater();
    }
    inprocessController_ = nullptr;
}

void InprocessDialog::initToolButton(QToolButton *button, const QString &text,
                                   const QString &iconPath) noexcept
{
    assert(button != nullptr);
    button->setText(text);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(30, 30));
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setFocusPolicy(Qt::NoFocus);
}

void InprocessDialog::addVerification() noexcept
{
    const auto isInMode = addVerificationButton_->isChecked();
    emit verificationModeChanged(isInMode);
    setVerificationMessageToScriptLabel(isInMode);
    handleVisibility();
}

void InprocessDialog::addComment() noexcept
{
    handleVisibility();
}

void InprocessDialog::pause() noexcept
{
    emit applicationPaused(true);
    pauseButton_->setVisible(false);
    playButton_->setVisible(true);
    setPlayPauseMessageToScriptLabel(true);
}

void InprocessDialog::play() noexcept
{
    emit applicationPaused(false);
    pauseButton_->setVisible(true);
    playButton_->setVisible(false);
    setPlayPauseMessageToScriptLabel(false);
}

void InprocessDialog::handleVisibility() noexcept
{
    commentWidget_->setVisible(addCommentButton_->isChecked());
    propertiesWatcher_->setVisible(addVerificationButton_->isChecked());
}

void InprocessDialog::acceptComment() noexcept
{
    emit newCommentLine(commentTextEdit_->toPlainText());
    clearComment();
}

void InprocessDialog::clearComment() noexcept
{
    commentTextEdit_->clear();
}

void InprocessDialog::handleNewScriptLine(const QString &scriptLine) noexcept
{
    //! TODO: не лучший вариант вычленения команды + сейчас довольно "тяжелая"
    //! функция tools::cutLine вызывается еще и в ScriptWriter
    const auto lines = tools::cutLine(scriptLine);
    for (const auto &line : lines) {
        if (line.startsWith('/')) {
            continue;
        }
        const auto bracketIndex = line.indexOf('(');
        assert(bracketIndex != -1);
        setTextToScriptLabel(line.left(bracketIndex));
        break;
    }
}

void InprocessDialog::setTextToScriptLabel(const QString &text) noexcept
{
    assert(scriptLineLabel_ != nullptr);
    if (needToRestoreLabelColor_) {
        needToRestoreLabelColor_ = false;
        setLabelTextColor();
    }
    scriptLineLabel_->setText(text);
}

void InprocessDialog::setPlayPauseMessageToScriptLabel(bool isPaused) noexcept
{
    assert(scriptLineLabel_ != nullptr);
    if (isPaused) {
        setLabelTextColor("#F8961E");
        scriptLineLabel_->setText("Recording paused. Please be cautious as your further actions in "
                                  "the application may lead to a non-functional test script.");
    }
    else {
        setLabelTextColor("#90BE6D");
        scriptLineLabel_->setText("Recording resumed.");
        needToRestoreLabelColor_ = true;
    }
}

void InprocessDialog::setVerificationMessageToScriptLabel(bool isInMode) noexcept
{
    assert(scriptLineLabel_ != nullptr);
    if (isInMode) {
        lastLabelText_ = scriptLineLabel_->text();
        setLabelTextColor("#43AA8B");
        scriptLineLabel_->setText(
            "You are in Verification Mode. Click on the desired component in your GUI.");
    }
    else {
        assert(!lastLabelText_.isEmpty());
        setLabelTextColor();
        scriptLineLabel_->setText(lastLabelText_);
    }
}

void InprocessDialog::setLabelTextColor(const QString &color) noexcept
{
    assert(scriptLineLabel_ != nullptr);
    if (color.isEmpty()) {
        scriptLineLabel_->setStyleSheet("");
    }
    else {
        scriptLineLabel_->setStyleSheet("QLabel { color : " + color + "; }");
    }
}
} // namespace QtAda::core::gui
