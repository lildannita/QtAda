#include "mainwindow.h"

#include <QApplication>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    QStringList args;
    args.reserve(argc);
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (!arg.startsWith("-qmljsdebugger")) {
            args.push_back(arg);
        }
    }

    bool isAutoRecord = false;
    bool isAutoUpdate = false;
    if (!args.isEmpty()) {
        for (const auto &arg : args) {
            if (arg == "--auto-record") {
                isAutoRecord = true;
            }
            else if (arg == "--auto-update") {
                isAutoUpdate = true;
            }
            else {
                std::cout << "Unknown argument: " << qPrintable(arg) << std::endl;
                return 1;
            }
        }
    }

    if (isAutoRecord || isAutoUpdate) {
        w.implementActionsForAutoRecord(isAutoUpdate);
    }

    return a.exec();
}
