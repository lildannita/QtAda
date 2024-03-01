#pragma once

#include <QObject>
#include <QDebug>
#include <QCoreApplication>

namespace qobj_example {
class MyCoreApplication : public QCoreApplication
{
    Q_OBJECT
public:
    MyCoreApplication(int &argc, char **argv) : QCoreApplication(argc, argv) {}
};

class ExampleObject : public QObject {
    Q_OBJECT
public:
    ExampleObject(QObject *parent = nullptr) noexcept
        : QObject{ parent }
    {
        qInfo() << "ExampleObject::constructor()";
    }

    ~ExampleObject() noexcept { qInfo() << "ExampleObject::destructor()"; }
};
} // namespace qobj_example
