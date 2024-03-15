#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class Dialog : public QDialog {
    Q_OBJECT

public:
    Dialog(bool hasOpenButton = false, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        auto *layout = new QVBoxLayout(this);
        auto *lineEdit = new QLineEdit(this);
        auto *okButton = new QPushButton("OK", this);

        layout->addWidget(lineEdit);
        layout->addWidget(okButton);

        connect(okButton, &QPushButton::clicked, this, &Dialog::accept);

        if (hasOpenButton) {
            auto *openButton = new QPushButton("Open", this);
            layout->addWidget(openButton);
            connect(openButton, &QPushButton::clicked, this, &Dialog::onOpenClicked);
        }
    }

private slots:
    void onOpenClicked()
    {
        Dialog *newDialog = new Dialog(false, this);
        newDialog->exec();
    }
};

#endif // DIALOG_H
