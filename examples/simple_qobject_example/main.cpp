#include "ExampleObject.hpp"

int main(int argc, char *argv[])
{
    qobj_example::MyCoreApplication app(argc, argv);
    qobj_example::ExampleObject obj(&app);
//    auto obj = new qobj_example::ExampleObject(&app);
//    obj->deleteLater();
    auto result = app.exec();
    return 0;
}
