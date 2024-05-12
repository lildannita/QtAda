#include "InprocessRunner.hpp"

#include <QRemoteObjectHost>

#include "InprocessController.hpp"
#include "Paths.hpp"

namespace QtAda::inprocess {
InprocessRunner::InprocessRunner(QObject *parent) noexcept
    : QObject{ parent }
    , inprocessHost_{ new QRemoteObjectHost(QUrl(paths::REMOTE_OBJECT_PATH), this) }
    , inprocessController_{ new InprocessController }
{
    connect(inprocessController_, &InprocessController::applicationRunningChanged, this,
            &InprocessRunner::handleApplicationStateChanged);
    connect(inprocessController_, &InprocessController::scriptRunError, this,
            &InprocessRunner::scriptRunError);
    connect(inprocessController_, &InprocessController::scriptRunWarning, this,
            &InprocessRunner::scriptRunWarning);
    connect(inprocessController_, &InprocessController::scriptRunLog, this,
            &InprocessRunner::scriptRunLog);
    inprocessHost_->enableRemoting(inprocessController_);
}

InprocessRunner::~InprocessRunner() noexcept
{
    if (inprocessController_ != nullptr) {
        inprocessController_->deleteLater();
    }
    inprocessController_ = nullptr;
}

void InprocessRunner::handleApplicationStateChanged(bool isAppRunning) noexcept
{
    // В отличие от InprocessDialog, который используется для записи скрипта,
    // для InprocessRunner завершение тестируемого приложения означает и завершение
    // работы InprocessRunner, поэтому сообщаем только о запуске приложения, что
    // важно для Launcher
    if (isAppRunning) {
        emit applicationStarted();
    }
}
} // namespace QtAda::inprocess
