#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolButton>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QUndoCommand>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
class QTreeWidgetItem;
QT_END_NAMESPACE

class MyCommand : public QUndoCommand {
public:
    MyCommand(uint8_t id, QLabel *label, QUndoCommand *parent = nullptr)
        : QUndoCommand(parent)
        , id_(id)
        , label_(label)
    {
        setText("Command №" + QString::number(id));
    }
    void undo() override
    {
        if (label_->text() == "Reseted") {
            label_->clear();
        }
        label_->setText(label_->text() + QStringLiteral("U(%1)|").arg(id_));
    }
    void redo() override
    {
        if (label_->text() == "Reseted") {
            label_->clear();
        }
        label_->setText(label_->text() + QStringLiteral("R(%1)|").arg(id_));
    }

private:
    QLabel *label_ = nullptr;
    uint8_t id_ = 0;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openFirstDialog();
    void openSecondDialog();
    void on_pushButton_clicked();
    void on_comboBox_currentTextChanged(const QString &arg1);
    void on_comboBox_2_currentIndexChanged(int index);
    void on_fontComboBox_editTextChanged(const QString &arg1);
    void on_spinBox_textChanged(const QString &arg1);
    void on_doubleSpinBox_valueChanged(double arg1);
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_buttonBox_helpRequested();
    void on_buttonBox_clicked(QAbstractButton *button);

    void on_pushButton_4_clicked();

private:
    Ui::MainWindow *ui;

    QStandardItemModel *tableViewModel;
    QStandardItemModel *treeViewModel;
    QStringListModel *listViewModel;
    QUndoStack *undoStack;
    QStandardItemModel *columnViewModel;
};

#endif // MAINWINDOW_H
