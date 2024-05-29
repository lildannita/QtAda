#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <iostream>

#include "QmlAutoRecord.hpp"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

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

    QmlAutoRecord autoRecord(&engine);
    if (isAutoRecord || isAutoUpdate) {
        autoRecord.implementActionsForAutoRecord(isAutoUpdate);
    }

    return app.exec();
}
