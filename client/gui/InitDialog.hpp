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
class InitDialog final : public QDialog {
    Q_OBJECT
public:
    InitDialog(QWidget *parent = nullptr) noexcept;
    ~InitDialog() = default;

    const QString selectedProjectPath() const noexcept
    {
        assert(!selectedProjectPath_.isEmpty());
        return selectedProjectPath_;
    }

private slots:
    void handleNewProject() noexcept;
    void handleOpenProject() noexcept;
    void handleRecentProject(const QModelIndex &index) noexcept;

private:
    QString selectedProjectPath_;

    QSettings *config_ = nullptr;
    QStandardItemModel *recentModel_ = nullptr;
    QListView *recentView_ = nullptr;
    QWidget *emptyRecentWidget_ = nullptr;

    QPushButton *initButton(const QString &text, const QString &iconPath) noexcept;
    void updateRecentModel() noexcept;

    bool checkProjectFilePath(const QString &path, bool isOpenMode = false,
                              bool needToShowMsg = true) noexcept;
    void updateRecentInConfig(const QString &path) noexcept;

    void acceptPath(const QString &path) noexcept;
};
} // namespace QtAda::gui
