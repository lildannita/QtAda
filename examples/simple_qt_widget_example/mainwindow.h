#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
class QStandardItemModel;
class QStringListModel;
class QUndoStack;
class QToolButton;
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr) noexcept;
    ~MainWindow() noexcept;
    void implementActionsForAutoRecord(bool isUpdate) noexcept;

private slots:
    void openFirstDialog() noexcept;
    void openSecondDialog() noexcept;

    void on_checkablePushButton_clicked(bool checked);
    void on_simplePushButton_clicked();

    void on_closeButton_clicked();

private:
    Ui::MainWindow *ui;

    QStandardItemModel *tableViewModel;
    QStandardItemModel *treeViewModel;
    QStandardItemModel *customListViewModel;
    QStringListModel *listViewModel;
    QUndoStack *undoStack;
    QStandardItemModel *columnViewModel;

    QToolButton *buttonForSimpleDialog = nullptr;
    QToolButton *buttonForAnotherDialog = nullptr;
    QToolButton *simpleToolButton = nullptr;

    // Используется только при --auto-record
    using Action = std::function<void()>;
    QVector<Action> testActions_;
    QTimer *testTimer_ = nullptr;
    bool testMode = false;
};
