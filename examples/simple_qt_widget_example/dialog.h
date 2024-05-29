#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTest>
#include <QTimer>

class Dialog : public QDialog {
    Q_OBJECT

public:
    Dialog(bool hasOpenButton = false, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        auto *layout = new QVBoxLayout(this);
        auto *lineEdit = new QLineEdit(this);
        okButton = new QPushButton("OK", this);

        layout->addWidget(lineEdit);
        layout->addWidget(okButton);

        connect(okButton, &QPushButton::clicked, this, &Dialog::accept);

        if (hasOpenButton) {
            openButton = new QPushButton("Open", this);
            layout->addWidget(openButton);
            connect(openButton, &QPushButton::clicked, this, &Dialog::onOpenClicked);
        }
    }

public slots:
    void autoClickOnOkButton() noexcept
    {
        // Автоматически нажимаем на кнопку "OK" (закрываем текущий диалог)
        QTest::mouseClick(okButton, Qt::LeftButton);
    }
    void autoClickOnOpenButton() noexcept
    {
        // Это нужно для дочернего диалога, чтобы указать обработчику onOpenClicked о том,
        // что находимся в режиме тестирования
        testMode = true;
        // Автоматически нажимаем на кнопку "Open" (открываем дочерний диалог)
        QTest::mouseClick(openButton, Qt::LeftButton);
    }

private slots:
    void onOpenClicked()
    {
        Dialog *newDialog = new Dialog(false, this);
        if (testMode) {
            // Если находимся в режиме тестирования, то
            QTimer::singleShot(1500, [this, newDialog] {
                // через 1,5 секунды закрываем дочерний диалог
                newDialog->autoClickOnOkButton();
                // и через еще 1,5 секунды закрываем старший диалог
                QTimer::singleShot(1500, [this] { this->autoClickOnOkButton(); });
            });
        }
        newDialog->exec();
    }

private:
    QPushButton *okButton = nullptr;
    QPushButton *openButton = nullptr;
    bool testMode = false;
};
