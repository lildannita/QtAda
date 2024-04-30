#include "GuiTools.hpp"

#include <QAbstractItemView>

#include "ProbeDetector.hpp"
#include "LauncherUtils.hpp"

namespace QtAda::gui::tools {
void deleteModels(QAbstractItemView *view) noexcept
{
    if (view == nullptr) {
        return;
    }

    auto selectionModel = view->selectionModel();
    if (selectionModel != nullptr) {
        selectionModel->deleteLater();
    }

    auto model = view->model();
    if (model != nullptr) {
        model->deleteLater();
    }
}

AppPathCheck checkProjectAppPath(const QString &path) noexcept
{
    const auto absExeAppPath = launcher::utils::absoluteExecutablePath(path);
    if (absExeAppPath.isEmpty()) {
        return AppPathCheck::NoExecutable;
    }

    const auto probeAbi = launcher::probe::detectProbeAbiForExecutable(absExeAppPath);
    if (!probeAbi.isValid()) {
        return AppPathCheck::NoProbe;
    }

    return AppPathCheck::Ok;
}

QString fileNameWithoutSuffix(const QString &path) noexcept
{
    const auto lastSlashIndex = path.lastIndexOf('/');
    const auto startIndex = lastSlashIndex != -1 ? lastSlashIndex + 1 : 0;
    const auto lastDotIndex = path.lastIndexOf('.');
    const auto result = path.mid(startIndex, lastDotIndex - startIndex).trimmed();
    assert(!result.isEmpty());
    return result;
}
} // namespace QtAda::gui::tools
