#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QToolButton;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;
QT_END_NAMESPACE

namespace QtAda::core::gui {
class ControlDialog : public QDialog {
    Q_OBJECT
public:
    explicit ControlDialog(QWidget *parent = nullptr) noexcept;

private:
    QToolButton *completeScriptButton_ = nullptr;
    QToolButton *addVerificationButton_ = nullptr;
    QToolButton *addCommentButton_ = nullptr;
    QToolButton *playPauseButton_ = nullptr;
    QToolButton *cancelScriptButton_ = nullptr;

    void initToolButton(QToolButton *button, const QString &text, const QString &iconPath) noexcept;
};
} // namespace QtAda::core::gui
