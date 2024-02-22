#include "ProcessInjector.hpp"

namespace launcher::injector {
ProcessInjector::ProcessInjector()
    : mExitCode(-1)
    , mProcessError(QProcess::UnknownError)
    , mExitStatus(QProcess::NormalExit)
{

}
}
