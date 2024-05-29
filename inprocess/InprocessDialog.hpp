#pragma once

#include <QDialog>

#include "Settings.hpp"

QT_BEGIN_NAMESPACE
class QToolButton;
class QLabel;
class QTextEdit;
class QRemoteObjectHost;
QT_END_NAMESPACE

namespace QtAda::inprocess {
class InprocessController;
class PropertiesWatcher;
class ScriptWriter;

class InprocessDialog final : public QDialog {
    Q_OBJECT
public:
    explicit InprocessDialog(const RecordSettings &settings, QWidget *parent = nullptr) noexcept;
    ~InprocessDialog() noexcept;

    void setTextToScriptLabel(const QString &text) noexcept;
    void setApplicationClosedExternally() noexcept
    {
        assert(applicationClosedExternally_ == false);
        applicationClosedExternally_ = true;
    }
    bool isStarted() const noexcept
    {
        return started_;
    }

public slots:
    void appendLogMessage(const QString &line) noexcept;
    void finishScript(bool isCancelled) noexcept;

signals:
    void applicationStarted();
    void inprocessClosed();

private slots:
    void handleApplicationStateChanged(bool isAppRunning) noexcept;

    void handleVerificationToggle(bool isChecked) noexcept;
    void handleCommentToggle(bool isChecked) noexcept;
    void handleLogToggle(bool isChecked) noexcept;
    void play() noexcept;
    void pause() noexcept;

    void acceptComment() noexcept;
    void clearComment() noexcept;

    void setNewCommandLine(const QString &command) noexcept;

private:
    bool started_ = false;
    bool applicationClosedExternally_ = false;

    QRemoteObjectHost *inprocessHost_ = nullptr;
    InprocessController *inprocessController_ = nullptr;

    ScriptWriter *scriptWriter_ = nullptr;

    QToolButton *verificationButton_ = nullptr;
    QToolButton *pauseButton_ = nullptr;
    QToolButton *playButton_ = nullptr;
    QToolButton *initButton(const QString &text, const QString &iconPath,
                            const QSize &minimumSize = QSize()) noexcept;

    QLabel *lineLabel_ = nullptr;
    QString lastLabelText_;

    PropertiesWatcher *propertiesWatcher_ = nullptr;

    QWidget *commentWidget_ = nullptr;
    QTextEdit *commentTextEdit_ = nullptr;

    QTextEdit *logTextArea_ = nullptr;

    bool needToRestoreLabelColor_ = false;

    void setPlayPauseMessageToScriptLabel(bool isPaused) noexcept;
    void setVerificationMessageToScriptLabel(bool isInMode) noexcept;
    void setLabelTextColor(const QString &color = QString()) noexcept;
};
} // namespace QtAda::inprocess
