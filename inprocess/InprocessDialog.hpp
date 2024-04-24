#pragma once

#include <QDialog>

#include "InprocessController.hpp"
#include "GenerationSettings.hpp"

QT_BEGIN_NAMESPACE
class QToolButton;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QTextEdit;
class QFrame;
QT_END_NAMESPACE

namespace QtAda::inprocess {
class PropertiesWatcher;
class ScriptWriter;

class InprocessDialog final : public QDialog {
    Q_OBJECT
public:
    explicit InprocessDialog(const common::GenerationSettings &settings,
                             QWidget *parent = nullptr) noexcept;
    ~InprocessDialog() noexcept;

private slots:
    //! TODO:
    void completeScript() noexcept
    {
    }
    void cancelScript() noexcept
    {
    }

    void addVerification() noexcept;
    void addComment() noexcept;
    void play() noexcept;
    void pause() noexcept;

    void acceptComment() noexcept;
    void clearComment() noexcept;

    void setNewCommandLine(const QString &command) noexcept;

private:
    InprocessController *inprocessController_ = nullptr;
    ScriptWriter *scriptWriter_ = nullptr;

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

    bool needToRestoreLabelColor_ = false;

    void initToolButton(QToolButton *button, const QString &text, const QString &iconPath) noexcept;
    void handleVisibility() noexcept;

    void setTextToScriptLabel(const QString &text) noexcept;
    void setPlayPauseMessageToScriptLabel(bool isPaused) noexcept;
    void setVerificationMessageToScriptLabel(bool isInMode) noexcept;
    void setLabelTextColor(const QString &color = QString()) noexcept;
};
} // namespace QtAda::inprocess
