#include "mainwindow.h"

#include <QToolButton>
#include <QUndoCommand>
#include <QLabel>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QtTest>
#include <QTimer>
#include <QVector>
#include <QEvent>

#include "ui_mainwindow.h"
#include "dialog.h"
#include "customDelegate.h"

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

MainWindow::MainWindow(QWidget *parent) noexcept
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , tableViewModel(new QStandardItemModel(5, 3, this))
    , treeViewModel(new QStandardItemModel(this))
    , columnViewModel(new QStandardItemModel(this))
    , customListViewModel(new QStandardItemModel(this))
    , listViewModel(new QStringListModel(this))
    , undoStack(new QUndoStack(this))
{
    ui->setupUi(this);

    ui->menuBar->addAction("Dynamic");

    // Инициализация QToolButtons
    for (int i = 0; i < 15; i++) {
        QToolButton *btn = new QToolButton(this);
        if (i == 0) {
            connect(btn, &QToolButton::clicked, this, &MainWindow::openFirstDialog);
            btn->setText("Open Simple Dialog");
            buttonForSimpleDialog = btn;
        }
        else if (i == 1) {
            connect(btn, &QToolButton::clicked, this, &MainWindow::openSecondDialog);
            btn->setText("Open Another Dialog");
            buttonForAnotherDialog = btn;
        }
        else {
            if (i == 2) {
                simpleToolButton = btn;
            }
            btn->setText("ToolButton" + QString::number(i));
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
    QTreeWidgetItem *rootWidget0
        = new QTreeWidgetItem(ui->treeWidget, QStringList() << "Элемент 0");
    QTreeWidgetItem *rootWidget1
        = new QTreeWidgetItem(ui->treeWidget, QStringList() << "Элемент 1");
    QTreeWidgetItem *childWidget0
        = new QTreeWidgetItem(rootWidget0, QStringList() << "Подэлемент 0");
    QTreeWidgetItem *childWidget1
        = new QTreeWidgetItem(rootWidget1, QStringList() << "Подэлемент 1");
    QTreeWidgetItem *subChildWidget0
        = new QTreeWidgetItem(childWidget0, QStringList() << "Подподэлемент 0");

    // Инициализация QListView и QListWidget
    QStringList list;
    for (int i = 0; i < 10; i++) {
        list << "Элемент " + QString::number(i);
        ui->listWidget->addItem(list.at(i));
    }
    listViewModel->setStringList(list);
    ui->listView->setModel(listViewModel);

    ui->custom_listView->setModel(customListViewModel);
    ui->custom_listView->setItemDelegate(new CustomItemDelegate(this));
    for (int i = 0; i < 5; ++i) {
        auto *item = new QStandardItem(QString("Название %1").arg(i));
        item->setData(QString("Значение %1").arg(i), Qt::UserRole);
        item->setData(QColor::fromHsv(rand() % 255, 255, 255, 255), Qt::BackgroundRole);
        customListViewModel->appendRow(item);
    }
    connect(ui->custom_listView, &QListView::clicked, this, &MainWindow::openFirstDialog);

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

MainWindow::~MainWindow() noexcept
{
    delete ui;
}

void MainWindow::openFirstDialog() noexcept
{
    Dialog *dialog = new Dialog(false, this);
    if (testMode) {
        QTimer::singleShot(1500, [this, dialog] { dialog->autoClickOnOkButton(); });
    }
    dialog->exec();
}

void MainWindow::openSecondDialog() noexcept
{
    Dialog *dialog = new Dialog(true, this);
    if (testMode) {
        QTimer::singleShot(1500, [this, dialog] { dialog->autoClickOnOpenButton(); });
    }
    dialog->exec();
}

void MainWindow::on_checkablePushButton_clicked(bool checked)
{
    ui->label->setText(
        QStringLiteral("Checkable Button Toggled: %1").arg(checked ? "true" : "false"));
}

void MainWindow::on_simplePushButton_clicked()
{
    ui->label->setText("Simple Button Clicked");
}

static QMouseEvent *simpleMouseEvent(const QEvent::Type type, const QPoint &pos,
                                     const Qt::MouseButton button = Qt::LeftButton) noexcept
{
    return new QMouseEvent(type, pos, button, button, Qt::NoModifier);
}

//! TODO: поменять на mouseDClick, когда (и если) его исправят
static void simulateDblClick(QObject *obj, const QPoint pos) noexcept
{
    QApplication::postEvent(obj, simpleMouseEvent(QEvent::MouseButtonPress, pos));
    QApplication::postEvent(obj, simpleMouseEvent(QEvent::MouseButtonRelease, pos));
    QApplication::postEvent(obj, simpleMouseEvent(QEvent::MouseButtonDblClick, pos));
    QApplication::postEvent(obj, simpleMouseEvent(QEvent::MouseButtonRelease, pos));
}

void MainWindow::implementActionsForAutoRecord() noexcept
{
    testMode = true;
    // Этот вектор - сценарий, который будет записан программой QtAda, и который
    // будет воспроизведен автоматически, без вмешательства пользователя
    testActions_ = {
        /********** ПЕРВАЯ СТРАНИЦА **********/
        // Открытие списка QComboBox
        [this] { QTest::mouseClick(ui->comboBox, Qt::LeftButton); },
        // Нажатие на элемент в списке QComboBox
        [this] {
            auto *popup = ui->comboBox->view();
            if (popup) {
                const auto index = ui->comboBox->model()->index(2, 0);
                const auto rect = popup->visualRect(index);
                QTest::mouseClick(popup->viewport(), Qt::LeftButton, Qt::NoModifier, rect.center());
            }
        },
        // Изменение текста в QComboBox
        [this] {
            ui->editableComboBox->clear();
            QTest::keyClicks(ui->editableComboBox, "New Text For ComboBox");
            QTest::mouseClick(ui->label, Qt::LeftButton);
        },
        // Открытие списка QComboBox
        [this] {
            // Нужно для того, чтобы при симуляции нажатии на новый элемент список
            // не листался, что происходит, если шрифт по-умолчанию находится в середине
            // или в конце списка.
            ui->fontComboBox->setCurrentIndex(0);
            const auto x = ui->fontComboBox->width() - 10;
            const auto y = ui->fontComboBox->height() / 2;
            QTest::mouseClick(ui->fontComboBox, Qt::LeftButton, Qt::NoModifier, { x, y });
        },
        // Нажатие на элемент в списке QComboBox
        [this] {
            auto *popup = ui->fontComboBox->view();
            if (popup) {
                const auto index = ui->fontComboBox->model()->index(2, 0);
                const auto rect = popup->visualRect(index);
                QTest::mouseClick(popup->viewport(), Qt::LeftButton, Qt::NoModifier, rect.center());
            }
        },

        // Запись правильного значения в QSpinBox
        [this] {
            ui->spinBox->clear();
            QTest::keyClicks(ui->spinBox, "22");
            QTest::mouseClick(ui->label, Qt::LeftButton);
        },
        // Увеличение значения на один шаг в QSpinBox
        [this] {
            const auto x = ui->spinBox->width() - 10;
            const auto y = ui->spinBox->height() / 2 - 5;
            QTest::mouseClick(ui->spinBox, Qt::LeftButton, Qt::NoModifier, { x, y });
        },
        // Запись правильного значения в QDoubleSpinBox
        [this] {
            ui->doubleSpinBox->clear();
            QTest::keyClicks(ui->doubleSpinBox, "3,22");
            QTest::mouseClick(ui->label, Qt::LeftButton);
        },
        // Уменьшение значения на один шаг в QDoubleSpinBox
        [this] {
            const auto x = ui->doubleSpinBox->width() - 10;
            const auto y = ui->doubleSpinBox->height() / 2 + 5;
            QTest::mouseClick(ui->doubleSpinBox, Qt::LeftButton, Qt::NoModifier, { x, y });
        },

        // Нажатие на первый RadioButton (который disabled)
        [this] {
            const auto minSize = ui->disabledRadioButton->minimumSizeHint();
            QTest::mouseClick(ui->disabledRadioButton, Qt::LeftButton, Qt::NoModifier,
                              { minSize.width() / 2, minSize.height() / 2 });
        },
        // Нажатие на второй RadioButton
        [this] {
            const auto minSize = ui->firstEnabledRadio->minimumSizeHint();
            QTest::mouseClick(ui->firstEnabledRadio, Qt::LeftButton, Qt::NoModifier,
                              { minSize.width() / 2, minSize.height() / 2 });
        },
        // Нажатие на третий RadioButton
        [this] {
            const auto minSize = ui->secondEnabledRadio->minimumSizeHint();
            QTest::mouseClick(ui->secondEnabledRadio, Qt::LeftButton, Qt::NoModifier,
                              { minSize.width() / 2, minSize.height() / 2 });
        },

        // Нажатие на первый CheckBox (переводим в состояние true)
        [this] {
            const auto minSize = ui->firstCheckBox->minimumSizeHint();
            QTest::mouseClick(ui->firstCheckBox, Qt::LeftButton, Qt::NoModifier,
                              { minSize.width() / 2, minSize.height() / 2 });
        },
        // Нажатие на третий CheckBox
        [this] {
            const auto minSize = ui->thirdCheckBox->minimumSizeHint();
            QTest::mouseClick(ui->thirdCheckBox, Qt::LeftButton, Qt::NoModifier,
                              { minSize.width() / 2, minSize.height() / 2 });
        },
        // Нажатие на первый CheckBox (переводим в состояние false)
        [this] {
            const auto minSize = ui->firstCheckBox->minimumSizeHint();
            QTest::mouseClick(ui->firstCheckBox, Qt::LeftButton, Qt::NoModifier,
                              { minSize.width() / 2, minSize.height() / 2 });
        },

        // Нажатие на кнопку "Checkable Button" (переводим в состояние true)
        [this] { QTest::mouseClick(ui->checkablePushButton, Qt::LeftButton); },
        // Нажатие на кнопку "Simple Button"
        [this] { QTest::mouseClick(ui->simplePushButton, Qt::LeftButton); },
        // Повторное нажатие на кнопку "Checkable Button" (переводим в состояние false)
        [this] { QTest::mouseClick(ui->checkablePushButton, Qt::LeftButton); },

        // Выбор даты в CalendarWidget
        [this] {
            // Вводим исходную дату
            QDate dateToSelect(2022, 2, 22);
            ui->calendarWidget->setSelectedDate(dateToSelect);

            auto *calendarView = ui->calendarWidget->findChild<QTableView *>();
            if (calendarView) {
                auto currentIndex = calendarView->currentIndex();
                auto index
                    = calendarView->model()->index(currentIndex.row(), currentIndex.column() + 1);
                auto rect = calendarView->visualRect(index);
                // Симуляция нажатия на 23.02.2022
                QTest::mouseClick(calendarView->viewport(), Qt::LeftButton, Qt::NoModifier,
                                  rect.center());
            }
        },
        // Более сложный выбор даты в CalendarWidget
        [this] {
            auto *calendarView = ui->calendarWidget->findChild<QTableView *>();
            if (calendarView) {
                auto index = calendarView->model()->index(1, 1);
                auto rect = calendarView->visualRect(index);
                // Симуляция нажатия на 31.01.2022
                QTest::mouseClick(calendarView->viewport(), Qt::LeftButton, Qt::NoModifier,
                                  rect.center());
            }
        },

        // Запись правильного значения в QDateTimeEdit
        [this] {
            ui->dateTimeEdit->clear();
            QTest::keyClicks(ui->dateTimeEdit, "01.02.2022 22:22");
            QTest::mouseClick(ui->label, Qt::LeftButton);
        },
        // Запись правильного значения в QTimeEdit
        [this] {
            ui->timeEdit->clear();
            QTest::keyClicks(ui->timeEdit, "00:22");
            QTest::mouseClick(ui->label, Qt::LeftButton);
        },
        // Запись правильного значения в QDateEdit
        [this] {
            ui->dateEdit->clear();
            QTest::keyClicks(ui->dateEdit, "01.02.2022");
            QTest::mouseClick(ui->label, Qt::LeftButton);
        },

        // Изменение значения в QDial (достаточно просто нажатия по середине компонента, что и
        // делает QTest::mouseClick)
        [this] { QTest::mouseClick(ui->dial, Qt::LeftButton); },
        // Изменение значения в QScrollBar (горизонтальный)
        [this] { QTest::mouseClick(ui->horizontalScrollBar, Qt::LeftButton); },
        // Изменение значения в QSlider (вертикальный)
        [this] { QTest::mouseClick(ui->verticalSlider, Qt::LeftButton); },

        // Открытие обычного диалога (дальнейшие действия автоматически выполняются "внутри"
        // диалога)
        [this] { QTest::mouseClick(buttonForSimpleDialog, Qt::LeftButton); },
        // Открытие диалога с дочерним диалогом (дальнейшие действия автоматически выполняются
        // "внутри" диалогов)
        [this] { QTest::mouseClick(buttonForAnotherDialog, Qt::LeftButton); },
        // Нажатие на обычный QToolButton
        [this] { QTest::mouseClick(simpleToolButton, Qt::LeftButton); },

        /********** ВТОРАЯ СТРАНИЦА **********/
        // Переключаем на вторую страницу
        [this] {
            auto tabBar = ui->tabWidget->tabBar();
            QTest::mouseClick(tabBar, Qt::LeftButton, Qt::NoModifier, tabBar->tabRect(1).center());
        },
        // Нажимаем на кнопку, которая находится на первой вкладке
        [this] { QTest::mouseClick(ui->toolBoxButton, Qt::LeftButton); },
        // Выбираем вторую вкладку
        [this] {
            //! TODO: не очень надежный способ получения кнопки QToolBox
            QTest::mouseClick(
                ui->toolBox->findChildren<QAbstractButton *>("qt_toolbox_toolboxbutton").at(1),
                Qt::LeftButton);
        },
        // Нажимаем на первый RadioButton
        [this] {
            auto minSize = ui->toolBoxFirstRadio->minimumSizeHint();
            QTest::mouseClick(ui->toolBoxFirstRadio, Qt::LeftButton, Qt::NoModifier,
                              { minSize.width() / 2, minSize.height() / 2 });
        },
        // Нажимаем на второй RadioButton
        [this] {
            auto minSize = ui->toolBoxSecondRadio->minimumSizeHint();
            QTest::mouseClick(ui->toolBoxSecondRadio, Qt::LeftButton, Qt::NoModifier,
                              { minSize.width() / 2, minSize.height() / 2 });
        },

        /********** ТРЕТЬЯ СТРАНИЦА **********/
        // Переключаем на третью страницу
        [this] {
            auto tabBar = ui->tabWidget->tabBar();
            QTest::mouseClick(tabBar, Qt::LeftButton, Qt::NoModifier, tabBar->tabRect(2).center());
        },
        // Устанавливаем текст в QTextEdit
        [this] {
            ui->textEdit->setFocus();
            QTest::keyClicks(ui->textEdit, "SampleText");
        },
        // Устанавливаем текст в QPlainTextEdit
        [this] {
            ui->plainTextEdit->setFocus();
            QTest::keyClicks(ui->plainTextEdit, "SampleText");
        },
        // Устанавливаем текст в QLineEdit
        [this] {
            ui->lineEdit->setFocus();
            QTest::keyClicks(ui->lineEdit, "SampleText");
        },

        /********** ЧЕТВЕРТАЯ СТРАНИЦА **********/
        // Переключаем на четвертую страницу
        [this] {
            auto tabBar = ui->tabWidget->tabBar();
            QTest::mouseClick(tabBar, Qt::LeftButton, Qt::NoModifier, tabBar->tabRect(3).center());
        },
        // Двойной клик по делегату
        [this] {
            auto index = tableViewModel->index(0, 0);
            auto clickPos = ui->tableView->visualRect(index).center();
            simulateDblClick(ui->tableView->viewport(), clickPos);
        },
        // Изменение текста в делегате
        [this] {
            auto editor = ui->tableView->findChild<QLineEdit *>();
            assert(editor != nullptr);
            QTest::keyClicks(editor, "Test Text");
        },
        // Клик по делегату
        [this] {
            auto index = tableViewModel->index(1, 1);
            auto clickPos = ui->tableView->visualRect(index).center();
            QTest::mouseClick(ui->tableView->viewport(), Qt::LeftButton, Qt::NoModifier, clickPos);
        },
        // Нажатие по заголовку столбца (под номером 3) QTableView
        [this] {
            auto *header = ui->tableView->horizontalHeader();
            // По факту - это размер одной секции
            auto sectionPosition = header->sectionPosition(1);
            // Так как нам нужны координаты середины третьей секции, то можно посчитать так:
            auto clickPos = QPoint(sectionPosition * 2.5, header->height() / 2);
            QTest::mouseClick(header->viewport(), Qt::LeftButton, Qt::NoModifier, clickPos);
        },
        // Нажатие по заголовку строки (под номером 4) QTableWidget
        [this] {
            auto *header = ui->tableWidget->verticalHeader();
            auto sectionPosition = header->sectionPosition(1);
            auto clickPos = QPoint(header->width() / 2, sectionPosition * 3.5);
            QTest::mouseClick(header->viewport(), Qt::LeftButton, Qt::NoModifier, clickPos);
        },

        /********** ПЯТАЯ СТРАНИЦА **********/
        // Переключаем на пятую страницу
        [this] {
            auto tabBar = ui->tabWidget->tabBar();
            QTest::mouseClick(tabBar, Qt::LeftButton, Qt::NoModifier, tabBar->tabRect(4).center());
        },
        // Раскрытие делегата (QTreeView)
        [this] {
            auto index = treeViewModel->index(0, 0);
            auto rect = ui->treeView->visualRect(index);
            auto clickPos = QPoint(rect.topLeft().x() - 5, rect.height() / 2);
            QTest::mouseClick(ui->treeView->viewport(), Qt::LeftButton, Qt::NoModifier, clickPos);
        },
        // Двойной клик по дочернему делегату (QTreeView)
        [this] {
            auto index = treeViewModel->index(0, 0, treeViewModel->index(0, 0));
            auto clickPos = ui->treeView->visualRect(index).center();
            simulateDblClick(ui->treeView->viewport(), clickPos);
        },
        // Изменение текста дочернего делегата (QTreeView)
        [this] {
            auto editor = ui->treeView->findChild<QLineEdit *>();
            assert(editor != nullptr);
            QTest::keyClicks(editor, "Test Text");
        },
        // Клик по делегату (QTreeView)
        [this] {
            auto index = treeViewModel->index(1, 0);
            auto clickPos = ui->treeView->visualRect(index).center();
            QTest::mouseClick(ui->treeView->viewport(), Qt::LeftButton, Qt::NoModifier, clickPos);
        },
        // Раскрытие делегата (QTreeWidget)
        [this] {
            auto rect = ui->treeWidget->visualItemRect(ui->treeWidget->topLevelItem(0));
            auto clickPos = rect.topLeft() + QPoint(-5, rect.height() / 2);
            QTest::mouseClick(ui->treeWidget->viewport(), Qt::LeftButton, Qt::NoModifier, clickPos);
        },
        // Сокрытие делегата (QTreeWidget)
        [this] {
            auto rect = ui->treeWidget->visualItemRect(ui->treeWidget->topLevelItem(0));
            auto clickPos = QPoint(rect.topLeft().x() - 5, rect.height() / 2);
            QTest::mouseClick(ui->treeWidget->viewport(), Qt::LeftButton, Qt::NoModifier, clickPos);
        },
        // Раскрытие дочернего делегата (QTreeView)
        [this] {
            auto index = treeViewModel->index(0, 0, treeViewModel->index(0, 0));
            auto rect = ui->treeView->visualRect(index);
            auto clickPos = rect.topLeft() + QPoint(-5, rect.height() / 2);
            QTest::mouseClick(ui->treeView->viewport(), Qt::LeftButton, Qt::NoModifier, clickPos);
        },

        /********** МЕНЮ **********/
        // Открываем меню
        [this] {
            auto *menu = this->menuBar()->actions().at(0);
            auto clickPos = this->menuBar()->actionGeometry(menu).center();
            QTest::mouseClick(this->menuBar(), Qt::LeftButton, Qt::NoModifier, clickPos);
        },
        // Нажимаем на первое "checkable" QAction
        [this] {
            auto *menu = this->menuBar()->actions().at(0)->menu();
            auto *action = menu->actions().at(0);
            auto clickPos = menu->actionGeometry(action).center();
            QTest::mouseClick(menu, Qt::LeftButton, Qt::NoModifier, clickPos);
        },
        // Открываем второе меню
        [this] {
            auto *menu = this->menuBar()->actions().at(1);
            auto clickPos = this->menuBar()->actionGeometry(menu).center();
            QTest::mouseClick(this->menuBar(), Qt::LeftButton, Qt::NoModifier, clickPos);
        },
        // Нажимаем на обычный QAction
        [this] {
            auto *menu = this->menuBar()->actions().at(1)->menu();
            auto *action = menu->actions().at(0);
            auto clickPos = menu->actionGeometry(action).center();
            QTest::mouseClick(menu, Qt::LeftButton, Qt::NoModifier, clickPos);
        },

    };

    testTimer_ = new QTimer(this);
    testTimer_->setInterval(500);
    connect(testTimer_, &QTimer::timeout, this, [this] {
        if (testActions_.empty()) {
            testTimer_->stop();
            QApplication::exit(0);
        }
        else {
            auto action = testActions_.takeFirst();
            action();
        }
    });

    // Откладываем запуск "сценария"
    QTimer::singleShot(1500, [this] { testTimer_->start(); });
}
