#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QString;
class QSettings;
QT_END_NAMESPACE

namespace Ui {
class ProjectDialog;
}

namespace QtAda::project {
class ProjectDialog : public QDialog {
    Q_OBJECT
public:
    ProjectDialog(const QString &path, QWidget *parent = nullptr) noexcept;
private:
    Ui::ProjectDialog *ui = nullptr;
    QSettings *project_ = nullptr;
    std::map<QString, QString> autMap_;


    void updateAutMap() noexcept;
};
}
