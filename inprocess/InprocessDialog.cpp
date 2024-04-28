#include "InprocessDialog.hpp"

#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QTextEdit>
#include <QRemoteObjectHost>

#include "InprocessController.hpp"
#include "InprocessTools.hpp"
#include "ScriptWriter.hpp"
#include "PropertiesWatcher.hpp"

namespace QtAda::inprocess {
InprocessDialog::InprocessDialog(const RecordSettings &settings, QWidget *parent) noexcept
    : QDialog{ parent }
    , inprocessHost_{ new QRemoteObjectHost(QUrl(REMOTE_OBJECT_PATH), this) }
    , inprocessController_{ new InprocessController }
    , propertiesWatcher_{ new PropertiesWatcher(inprocessController_, this) }
    , scriptWriter_{ new ScriptWriter(settings, this) }
    , lineLabel_{ new QLabel(this) }
    , commentWidget_{ new QWidget(this) }
    , commentTextEdit_{ new QTextEdit(this) }
{
    connect(inprocessController_, &InprocessController::applicationStarted, this,
            &InprocessDialog::showDialog);
    connect(inprocessController_, &InprocessController::newScriptLine, scriptWriter_,
            &ScriptWriter::handleNewLine);
    connect(propertiesWatcher_, &PropertiesWatcher::newMetaPropertyVerification, scriptWriter_,
            &ScriptWriter::handleNewMetaPropertyVerification);
    connect(scriptWriter_, &ScriptWriter::newScriptCommandDetected, this,
            &InprocessDialog::setTextToScriptLabel);

    inprocessHost_->enableRemoting(inprocessController_);

    // Настройка параметров окна диалога
    this->setWindowTitle("QtAda | Control Bar");
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    this->setWindowIcon(QIcon(":/icons/app.png"));
    this->setAttribute(Qt::WA_DeleteOnClose);

    // Инициализация основных кнопок
    auto *cancelButton = initButton("Cancel\nscript", ":/icons/cancel.svg");
    const auto minimumButtonSize = cancelButton->sizeHint();
    auto *completeButton
        = initButton("Complete\nScript", ":/icons/complete.svg", minimumButtonSize);
    verificationButton_
        = initButton("Add\nVerification", ":/icons/verification.svg", minimumButtonSize);
    verificationButton_->setCheckable(true);
    auto *commentButton = initButton("Add\nComment", ":/icons/comment.svg", minimumButtonSize);
    commentButton->setCheckable(true);
    auto *logButton = initButton("View\nLog", ":/icons/log.svg", minimumButtonSize);
    logButton->setCheckable(true);
    pauseButton_ = initButton("Pause", ":/icons/pause.svg", minimumButtonSize);
    playButton_ = initButton("Play", ":/icons/play.svg", minimumButtonSize);
    playButton_->setVisible(false);
    // Подключение слотов к основным кнопкам
    connect(completeButton, &QToolButton::clicked, this, &InprocessDialog::completeScript);
    connect(verificationButton_, &QToolButton::toggled, this,
            &InprocessDialog::handleVerificationToggle);
    connect(commentButton, &QToolButton::toggled, this, &InprocessDialog::handleCommentToggle);
    connect(logButton, &QToolButton::toggled, this, &InprocessDialog::handleLogToggle);
    connect(pauseButton_, &QToolButton::clicked, this, &InprocessDialog::pause);
    connect(playButton_, &QToolButton::clicked, this, &InprocessDialog::play);
    connect(cancelButton, &QToolButton::clicked, this, &InprocessDialog::cancelScript);
    // Инициализация макета с кнопками
    auto *buttons = new QWidget(this);
    auto *buttonLayout = new QHBoxLayout(buttons);
    buttonLayout->addWidget(completeButton);
    buttonLayout->addWidget(verificationButton_);
    buttonLayout->addWidget(commentButton);
    buttonLayout->addWidget(tools::initSeparator(this));
    buttonLayout->addWidget(logButton);
    buttonLayout->addWidget(tools::initSeparator(this));
    buttonLayout->addWidget(pauseButton_);
    buttonLayout->addWidget(playButton_);
    buttonLayout->addWidget(cancelButton);

    // Инициализация строки, отображающей последнюю сгенерированную команду
    lineLabel_->setTextFormat(Qt::PlainText);
    lineLabel_->setWordWrap(true);
    setTextToScriptLabel("Start interacting in the application under test!");
    // Инициализация макета для строки
    auto *labelWidget = new QWidget(this);
    auto *scriptLayout = new QVBoxLayout(labelWidget);
    scriptLayout->addWidget(tools::initSeparator(this, true));
    scriptLayout->addWidget(lineLabel_);

    // Инициализация кнопок для управления комментарием
    auto *acceptCommentButton = new QPushButton(this);
    auto *clearCommentButton = new QPushButton(this);
    acceptCommentButton->setText("Accept");
    acceptCommentButton->setFocusPolicy(Qt::NoFocus);
    clearCommentButton->setText("Clear");
    clearCommentButton->setFocusPolicy(Qt::NoFocus);
    connect(acceptCommentButton, &QPushButton::clicked, this, &InprocessDialog::acceptComment);
    connect(clearCommentButton, &QPushButton::clicked, this, &InprocessDialog::clearComment);
    // Инициализация макета под кнопки для управления комментарием
    auto *commentButtons = new QWidget(this);
    auto *commentButtonsLayout = new QVBoxLayout(commentButtons);
    commentButtonsLayout->addWidget(acceptCommentButton);
    commentButtonsLayout->addWidget(clearCommentButton);
    // Инициализация текстового редактора для ввода комментария
    commentTextEdit_->setPlaceholderText("Enter comment text...");
    commentTextEdit_->setFixedHeight(commentButtonsLayout->sizeHint().height());
    // Инициализация макета для ввода комментариев
    auto *commentLayout = new QHBoxLayout(commentWidget_);
    commentLayout->addWidget(commentTextEdit_);
    commentLayout->addWidget(commentButtons);
    commentWidget_->setVisible(false);

    // Инициализация основного макета
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(buttons);
    mainLayout->addWidget(propertiesWatcher_);
    mainLayout->addWidget(commentWidget_);
    mainLayout->addWidget(labelWidget);

    this->adjustSize();
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    //! TODO: Костыль, см. InprocessController::startInitServer().
    inprocessController_->startInitServer();
}

InprocessDialog::~InprocessDialog() noexcept
{
    if (inprocessController_ != nullptr) {
        inprocessController_->deleteLater();
    }
    inprocessController_ = nullptr;
}

QToolButton *InprocessDialog::initButton(const QString &text, const QString &iconPath,
                                         const QSize &minimumSize) noexcept
{
    auto *button = new QToolButton(this);
    button->setText(text);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(30, 30));
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setFocusPolicy(Qt::NoFocus);
    if (minimumSize.isValid()) {
        button->setMinimumSize(minimumSize);
    }
    return button;
}

void InprocessDialog::showDialog() noexcept
{
    const auto screenGeometry = QApplication::screenAt(QCursor::pos())->geometry();
    this->move(screenGeometry.left() + screenGeometry.width() - this->width(),
               screenGeometry.top());
    this->show();
    this->raise();

    emit applicationStarted();
}

void InprocessDialog::handleVerificationToggle(bool isChecked) noexcept
{
    emit inprocessController_->verificationModeChanged(isChecked);
    setVerificationMessageToScriptLabel(isChecked);
    propertiesWatcher_->setVisible(isChecked);
    if (!isChecked) {
        propertiesWatcher_->clear();
    }
}

void InprocessDialog::handleCommentToggle(bool isChecked) noexcept
{
    commentWidget_->setVisible(isChecked);
}

void InprocessDialog::handleLogToggle(bool isChecked) noexcept
{
}

void InprocessDialog::pause() noexcept
{
    pauseButton_->setVisible(false);
    playButton_->setVisible(true);
    setPlayPauseMessageToScriptLabel(true);
    emit inprocessController_->applicationPaused(true);

    verificationButton_->setEnabled(false);
    if (propertiesWatcher_->isVisible()) {
        propertiesWatcher_->setVisible(false);
        propertiesWatcher_->clear();
    }
}

void InprocessDialog::play() noexcept
{
    playButton_->setVisible(false);
    pauseButton_->setVisible(true);
    setPlayPauseMessageToScriptLabel(false);
    emit inprocessController_->applicationPaused(false);

    verificationButton_->setEnabled(true);
    const auto propertiesWasVisible = verificationButton_->isChecked();
    if (propertiesWasVisible) {
        propertiesWatcher_->setVisible(propertiesWasVisible);
        QTimer::singleShot(1000, this, [this] { setVerificationMessageToScriptLabel(true); });
    }
}

void InprocessDialog::acceptComment() noexcept
{
    scriptWriter_->handleNewComment(commentTextEdit_->toPlainText());
    clearComment();
}

void InprocessDialog::clearComment() noexcept
{
    commentTextEdit_->clear();
}

void InprocessDialog::setNewCommandLine(const QString &command) noexcept
{
    setTextToScriptLabel(command);
}

void InprocessDialog::setTextToScriptLabel(const QString &text) noexcept
{
    assert(lineLabel_ != nullptr);
    if (needToRestoreLabelColor_) {
        needToRestoreLabelColor_ = false;
        setLabelTextColor();
    }
    lineLabel_->setText(text);
}

void InprocessDialog::setPlayPauseMessageToScriptLabel(bool isPaused) noexcept
{
    assert(lineLabel_ != nullptr);
    if (isPaused) {
        setLabelTextColor("#F8961E");
        lineLabel_->setText("Recording paused. Please be cautious as your further actions in "
                            "the application may lead to a non-functional test script.");
    }
    else {
        setLabelTextColor("#90BE6D");
        lineLabel_->setText("Recording resumed.");
        needToRestoreLabelColor_ = true;
    }
}

void InprocessDialog::setVerificationMessageToScriptLabel(bool isInMode) noexcept
{
    assert(lineLabel_ != nullptr);
    if (isInMode) {
        lastLabelText_ = lineLabel_->text();
        setLabelTextColor("#43AA8B");
        lineLabel_->setText(
            "You are in Verification Mode. Click on the desired component in your GUI.");
    }
    else {
        assert(!lastLabelText_.isEmpty());
        setLabelTextColor();
        lineLabel_->setText(lastLabelText_);
    }
}

void InprocessDialog::setLabelTextColor(const QString &color) noexcept
{
    assert(lineLabel_ != nullptr);
    if (color.isEmpty()) {
        lineLabel_->setStyleSheet("");
    }
    else {
        lineLabel_->setStyleSheet("QLabel { color : " + color + "; }");
    }
}

void InprocessDialog::completeScript() noexcept
{
    emit inprocessClosed();
    this->close();
}

void InprocessDialog::cancelScript() noexcept
{
    scriptWriter_->handleCancelledScript();
    completeScript();
}
} // namespace QtAda::inprocess
