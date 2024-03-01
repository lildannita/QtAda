#pragma once

#include <QObject>
#include <QDebug>

namespace qobj_example {
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
