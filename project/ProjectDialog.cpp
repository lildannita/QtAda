#include "ProjectDialog.hpp"

#include <QDir>
#include <QSettings>

#include "CommonData.hpp"
#include "CommonFunctions.hpp"
#include <ui_ProjectDialog.h>

namespace QtAda::project {
static void setIconForInfoLabel(const QLatin1String &iconPath, QLabel *icon) noexcept
{
    icon->setPixmap(QPixmap(iconPath).scaledToHeight(icon->height() * 0.8, Qt::SmoothTransformation));
}

ProjectDialog::ProjectDialog(const QString &path, QWidget *parent) noexcept
    : QDialog{ parent }
    , ui{ new Ui::ProjectDialog }
{
    ui->setupUi(this);

    // Setup icons for 'info' labels
    setIconForInfoLabel(QLatin1String(":/project/project.svg"), ui->projectIcon);
    setIconForInfoLabel(QLatin1String(":/project/aut.svg"), ui->autIcon);
    setIconForInfoLabel(QLatin1String(":/project/script.svg"), ui->tsIcon);

    // Set project name from path
    assert(path.endsWith(PROJECT_FORMAT));
    constexpr auto formatLength = strLength(PROJECT_FORMAT);
    const auto sepIndex = path.lastIndexOf(QDir::separator());
    assert(sepIndex != -1);
    const auto projectName = path.mid(sepIndex + 1, path.length() - sepIndex - formatLength - 1);
    ui->projectEdit->setText(std::move(projectName));

    // Open and read project file
    project_ = new QSettings(path, QSettings::IniFormat);

}

void ProjectDialog::updateAutMap() noexcept
{
    assert(project_ != nullptr);

    project_->beginGroup(PROJECT_AUT_GROUP);
    const auto autPaths = project_->allKeys();
    for (const auto &autPath : autPaths) {

    }
    project_->endGroup();
}
}
