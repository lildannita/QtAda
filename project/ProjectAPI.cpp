#include "ProjectAPI.hpp"

#include <QFileInfo>
#include <QMessageBox>

#include "CommonData.hpp"

namespace QtAda::project {
void ProjectAPI::showMessage(const QString &msg, MsgType type) noexcept
{
    if (appParent_ == nullptr) {
        printMessage(msg, type);
        return;
    }

    switch (type) {
    case MsgType::Error: {
        QMessageBox::critical(appParent_, tr(HEADER_PROJECT_ERR), msg);
        break;
    }
    case MsgType::Warning: {
        QMessageBox::warning(appParent_, tr(HEADER_PROJECT_WRN), msg);
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}

bool ProjectAPI::setProject(const QString &path) noexcept
{
    QFileInfo projectInfo(path);

    if (!projectInfo.exists() || !projectInfo.isFile()) {
        showMessage(tr("The project file is not accessible."), MsgType::Error);
        return false;
    }
    else if (!projectInfo.isReadable() || !projectInfo.isWritable()) {
        showMessage(tr("The project file does not have the necessary read/write permissions."), MsgType::Error);
        return false;
    }
    else if (projectInfo.suffix() != PROJECT_FORMAT) {
        showMessage(tr("The project file format is incorrect."), MsgType::Error);
        return false;
    }

    return true;
}
}
