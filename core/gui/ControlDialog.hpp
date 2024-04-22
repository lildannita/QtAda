#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QToolButton;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QTextEdit;
class QFrame;
QT_END_NAMESPACE

namespace QtAda::core::gui {
class PropertiesWatcher;

class ControlDialog : public QDialog {
    Q_OBJECT
public:
    explicit ControlDialog(bool closeWindowsOnExit, QWidget *parent = nullptr) noexcept;

signals:
    void newCommentLine(const QString &comment);
    void applicationPaused(bool isPaused);
    void scriptCancelled();
    void verificationModeChanged(bool isVerificationMode);

    void objectSelectedInGui(const QObject *object);
    void frameCreatedInGui(const QObject *frame);
    void frameDestroyedInGui();
    void framedObjectChangedFromWatcher(QObject *framedObject);

public slots:
    void handleNewScriptLine(const QString &scriptLine) noexcept;

private slots:
    void completeScript() noexcept;
    void addVerification() noexcept;
    void addComment() noexcept;
    void play() noexcept;
    void pause() noexcept;
    void cancelScript() noexcept;

    void acceptComment() noexcept;
    void clearComment() noexcept;

private:
    QToolButton *completeScriptButton_ = nullptr;
    QToolButton *addVerificationButton_ = nullptr;
    QToolButton *addCommentButton_ = nullptr;
    QToolButton *pauseButton_ = nullptr;
    QToolButton *playButton_ = nullptr;
    QToolButton *cancelScriptButton_ = nullptr;

    QWidget *scriptWidget_ = nullptr;
    QLabel *scriptLineLabel_ = nullptr;
    QString lastLabelText_ = QString();

    PropertiesWatcher *propertiesWatcher_ = nullptr;

    QWidget *commentWidget_ = nullptr;
    QTextEdit *commentTextEdit_ = nullptr;
    QPushButton *acceptCommentButton_ = nullptr;
    QPushButton *clearCommentButton_ = nullptr;

    const bool closeWindowsOnExit_;
    bool needToRestoreLabelColor_ = false;

    void initToolButton(QToolButton *button, const QString &text, const QString &iconPath) noexcept;
    void handleVisibility() noexcept;

    void setTextToScriptLabel(const QString &text) noexcept;
    void setPlayPauseMessageToScriptLabel(bool isPaused) noexcept;
    void setVerificationMessageToScriptLabel(bool isInMode) noexcept;
    void setLabelTextColor(const QString &color = QString()) noexcept;
};
} // namespace QtAda::core::gui
