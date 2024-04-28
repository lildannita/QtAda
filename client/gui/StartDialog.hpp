#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
class QStandardItem;
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
    QPushButton *initButton(const QString &text, const QString &iconPath) noexcept;
    QStandardItem *initRecentItem(const QString &text) noexcept;
};
} // namespace QtAda::gui
