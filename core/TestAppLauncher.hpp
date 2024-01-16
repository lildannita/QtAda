#pragma once

#include <QObject>
#include <QProcess>

class TestAppLauncher final : public QObject
{
public:
    explicit TestAppLauncher(const QString &pathToBin, QObject *parent = nullptr) noexcept;
private:
    QProcess testApp_;
};

