#pragma once

#include <QObject>
#include <QWidget>

#include "CommonFunctions.hpp"

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace QtAda::project {
class ProjectAPI : public QObject {
    Q_OBJECT
public:
    ProjectAPI(QObject *parent = nullptr) : QObject{ parent } {}
    ProjectAPI(QWidget *parent = nullptr) : QObject{ parent }, appParent_{ parent } {}
    bool setProject(const QString &path) noexcept;

private:
    QWidget *appParent_ = nullptr;
    QSettings *project_ = nullptr;
    std::map<QString, QString> autMap_;
    std::vector<QString> tsVector_;

    void showMessage(const QString &msg, MsgType type) noexcept;
};
}
