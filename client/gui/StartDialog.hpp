#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QSettings;
class QPushButton;
class QStandardItemModel;
class QListView;
class QLabel;
class QString;
QT_END_NAMESPACE

namespace QtAda::gui {
class StartDialog final : public QDialog {
    Q_OBJECT
public:
    StartDialog(QWidget *parent = nullptr);
    ~StartDialog();

private slots:
    void handleNewProject() noexcept;
    void handleOpenProject() noexcept;
    void handleRecentProject(const QModelIndex &index) noexcept;

private:
    QSettings *config_ = nullptr;
    QStandardItemModel *recentModel_ = nullptr;
    QListView *recentView_ = nullptr;
    QWidget *emptyRecentWidget_ = nullptr;

    QPushButton *initButton(const QString &text, const QString &iconPath) noexcept;
    void updateRecentModel() noexcept;

    bool checkProjectFilePath(const QString &path, bool isOpenMode = false,
                              bool needToShowMsg = true) noexcept;
    void updateRecentInConfig(const QString &path) noexcept;
};
} // namespace QtAda::gui
