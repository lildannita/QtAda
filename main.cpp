#include "launcher/ProbeDetector.hpp"

#include <QStringList>

int main(int argc, char *argv[])
{
//    QCoreApplication::setApplicationName(QStringLiteral("QtAda"));
//#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
//    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
//#endif

    QStringList args;
    args.reserve(argc);
    for (int i = 1; i < argc; ++i)
        args.push_back(QString::fromLocal8Bit(argv[i]));

//    const auto test = launcher::probe::detectProbeAbiForExecutable("/files/mgtu/android_currencyconverter/build-client-Desktop-Debug/client");

    return 0;
}
