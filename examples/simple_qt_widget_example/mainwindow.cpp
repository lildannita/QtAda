#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , tableViewModel(new QStandardItemModel(5, 3, this))
    , treeViewModel(new QStandardItemModel(this))
    , columnViewModel(new QStandardItemModel(this))
    , listViewModel(new QStringListModel(this))
    , undoStack(new QUndoStack(this))
{
    ui->setupUi(this);

    // Инициализация QToolButtons
    for (int i = 0; i < 15; i++) {
        QToolButton *btn = new QToolButton(this);
        btn->setText("ToolButton" + QString::number(i));
        if (i == 0) {
            connect(btn, &QToolButton::clicked, this, &MainWindow::openFirstDialog);
        }
        if (i == 1) {
            connect(btn, &QToolButton::clicked, this, &MainWindow::openSecondDialog);
        }
        ui->toolBar->addWidget(btn);
    }

    // Инициализация QTableView и QTableWidget
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setRowCount(5);
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "Столбец 1"
                                                             << "Столбец 2");
    for (int row = 0; row < 5; ++row) {
        for (int column = 0; column < 3; ++column) {
            const auto cellName = QString("Ячейка %1, %2").arg(row).arg(column);
            QStandardItem *item = new QStandardItem(cellName);
            tableViewModel->setItem(row, column, item);

            ui->tableWidget->setItem(row, column, new QTableWidgetItem(cellName));
        }
    }
    ui->tableView->setModel(tableViewModel);

    // Инициализация QTreeView
    QStandardItem *rootItem0 = new QStandardItem("Элемент 0");
    QStandardItem *rootItem1 = new QStandardItem("Элемент 1");
    QStandardItem *childItem0 = new QStandardItem("Подэлемент 0");
    QStandardItem *childItem1 = new QStandardItem("Подэлемент 1");
    QStandardItem *subChildItem0 = new QStandardItem("Подподэлемент 0");
    childItem0->appendRow(subChildItem0);
    rootItem0->appendRow(childItem0);
    rootItem1->appendRow(childItem1);
    treeViewModel->invisibleRootItem()->appendRow(rootItem0);
    treeViewModel->invisibleRootItem()->appendRow(rootItem1);
    ui->treeView->setModel(treeViewModel);

    // Инициализация QTreeWidget
    QTreeWidgetItem *rootWidget0 = new QTreeWidgetItem(ui->treeWidget, QStringList() << "Элемент 0");
    QTreeWidgetItem *rootWidget1 = new QTreeWidgetItem(ui->treeWidget, QStringList() << "Элемент 1");
    QTreeWidgetItem *childWidget0 = new QTreeWidgetItem(rootWidget0, QStringList() << "Подэлемент 0");
    QTreeWidgetItem *childWidget1 = new QTreeWidgetItem(rootWidget1, QStringList() << "Подэлемент 1");
    QTreeWidgetItem *subChildWidget0 = new QTreeWidgetItem(childWidget0, QStringList() << "Подподэлемент 0");

    // Инициализация QListView и QListWidget
    QStringList list;
    for (int i = 0; i < 10; i++) {
        list << "Элемент " + QString::number(i);
        ui->listWidget->addItem(list.at(i));
    }
    listViewModel->setStringList(list);
    ui->listView->setModel(listViewModel);

    // Инициализация QUndoView
    for (int i = 0; i < 5; i++) {
        auto cmd = new MyCommand(i, ui->undoAndColumnLabel);
        undoStack->push(cmd);
    }
    ui->undoView->setStack(undoStack);

    // Инициализация QColumnView
    QStandardItem *rootItem = columnViewModel->invisibleRootItem();
    QStandardItem *item0 = new QStandardItem("Пункт 0");
    QStandardItem *item1 = new QStandardItem("Пункт 1");
    rootItem->appendRow(item0);
    rootItem->appendRow(item1);
    QStandardItem *subItem0 = new QStandardItem("Подпункт 0");
    item0->appendRow(subItem0);
    QStandardItem *subItem1 = new QStandardItem("Подпункт 1");
    item1->appendRow(subItem1);
    subItem0->appendRow(new QStandardItem("Подподпункт 0"));
    ui->columnView->setModel(columnViewModel);

    // Ввод текста в QTextBrowser и обработка нажатия на ссылку
    ui->textBrowser->setHtml(R"(
        <h1>Пример ссылки</h1>
        <p>Это пример текста с <a href="action:show_message">кликабельной ссылкой</a>.</p>
    )");
    QObject::connect(ui->textBrowser, &QTextBrowser::anchorClicked, [this](const QUrl &url) {
        if (url.scheme() == "action" && url.path() == "show_message") {
            ui->label->setText("Link clicked -> TextBrowser::readOnly = false");
            ui->textBrowser->setReadOnly(false);
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openFirstDialog()
{
    Dialog *dialog = new Dialog(false, this);
    dialog->exec();
}

void MainWindow::openSecondDialog()
{
    Dialog *dialog = new Dialog(true, this);
    dialog->exec();
}

void MainWindow::on_pushButton_clicked()
{
    ui->label->setText("Simple Button clicked");
}

void MainWindow::on_comboBox_currentTextChanged(const QString &arg1)
{
    ui->label->setText("ComboBox1 text -> " + arg1);
}

void MainWindow::on_comboBox_2_currentIndexChanged(int index)
{
    ui->label->setText("ComboBox2 index -> " + QString::number(index));
}

void MainWindow::on_fontComboBox_editTextChanged(const QString &arg1)
{
    ui->label->setText("FontComboBox index -> " + arg1);
}

void MainWindow::on_spinBox_textChanged(const QString &arg1)
{
    ui->label->setText("SpinBox text -> " + arg1);
}

void MainWindow::on_doubleSpinBox_valueChanged(double arg1)
{
    ui->label->setText("DoubleSpinBox value -> " + QString::number(arg1, 'f', 2));
}

void MainWindow::on_buttonBox_accepted()
{
    ui->label->setText("ButtonBox accepted");
}

void MainWindow::on_buttonBox_rejected()
{
    ui->label->setText("ButtonBox rejected");
}

void MainWindow::on_buttonBox_helpRequested()
{
    ui->label->setText("ButtonBox help requested");
}

void MainWindow::on_buttonBox_clicked(QAbstractButton *button)
{
    ui->label->setText("ButtonBox clicked");
}

void MainWindow::on_pushButton_4_clicked()
{
    ui->undoAndColumnLabel->setText("Reseted");
}
