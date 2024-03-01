#include "ExampleObject.hpp"

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    auto *obj = new qobj_example::ExampleObject(&app);
    obj->deleteLater();
    auto result = app.exec();
    return result;
}
