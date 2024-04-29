#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QSettings;
class QPushButton;
class QStandardItemModel;
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
    void handleRecentProject() noexcept;

private:
    QSettings *config_ = nullptr;

    QPushButton *initButton(const QString &text, const QString &iconPath) noexcept;
    QStandardItemModel *initRecentModel() noexcept;
};
} // namespace QtAda::gui
