#include "ControlDialog.hpp"

#include <QCoreApplication>
#include <QApplication>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

namespace QtAda::core::gui {
ControlDialog::ControlDialog(QWidget *parent) noexcept
    : QDialog{ parent }
    , completeScriptButton_{ new QToolButton(this) }
    , addVerificationButton_{ new QToolButton(this) }
    , addCommentButton_{ new QToolButton(this) }
    , playPauseButton_{ new QToolButton(this) }
    , cancelScriptButton_{ new QToolButton(this) }
{
    this->setWindowTitle("QtAda | Control Bar");
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    initToolButton(completeScriptButton_, "Complete Script", ":/icons/scenario_ready.svg");
    initToolButton(addVerificationButton_, "Add Verification", ":/icons/add_verification.svg");
    initToolButton(addCommentButton_, "Add Comment", ":/icons/add_comment.svg");
    initToolButton(playPauseButton_, "Pause", ":/icons/pause.svg");
    initToolButton(cancelScriptButton_, "Cancel script", ":/icons/scenario_cancel.svg");

    QFrame *separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);

    QHBoxLayout *buttonLayout = new QHBoxLayout(this);
    buttonLayout->addWidget(completeScriptButton_);
    buttonLayout->addWidget(addVerificationButton_);
    buttonLayout->addWidget(addCommentButton_);
    buttonLayout->addWidget(separator);
    buttonLayout->addWidget(playPauseButton_);
    buttonLayout->addWidget(cancelScriptButton_);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(buttonLayout);

    this->setLayout(mainLayout);
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);
    // Иначе фокус будет все время на первой кнопке
    this->setFocus();

    connect(completeScriptButton_, &QToolButton::clicked, this,
            []() { QCoreApplication::exit(0); });
}

void ControlDialog::initToolButton(QToolButton *button, const QString &text,
                                   const QString &iconPath) noexcept
{
    assert(button != nullptr);
    button->setText(text);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(30, 30));
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}
} // namespace QtAda::core::gui
