#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QRemoteObjectHost;
QT_END_NAMESPACE

namespace QtAda::inprocess {
class InprocessController;

class InprocessRunner final : public QObject {
    Q_OBJECT
public:
    InprocessRunner(QObject *parent = nullptr) noexcept;
    ~InprocessRunner() noexcept;

signals:
    void applicationStarted();

    void scriptRunError(const QString &msg);
    void scriptRunLog(const QString &msg);

private slots:
    void handleApplicationStateChanged(bool isAppRunning) noexcept;

private:
    QRemoteObjectHost *inprocessHost_ = nullptr;
    InprocessController *inprocessController_ = nullptr;
};
} // namespace QtAda::inprocess
