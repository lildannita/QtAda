/*
 * ProbeAbi (abiForExecutable)
 *  0. get all dependies for exePath (need to make another class with shared functions) by ldd
 *
 *  1. contains from ldd:
 *      contains QtCore
 *      contains QtQuick (QtQml?)
 *          contains prefix (lib)
 *          contains suffix (Qt)
 *
 *     // Path must not be something like "libqtqmlcoreplugin" which is not what we're looking for
        if (line.contains(QByteArrayLiteral("qml"))) {
            return false;
        }
 *
 *  2. get version and architecture:
 *      if in libPath no version -> execuye .so lib and find version
 *      arch: if HAVE_ELF -> arch, if no -> empty
 *
 *
 *
 *
 * RESULT = ProbeABI
 */
#include "ProbeABI.hpp"

class QString;

namespace launcher::probe {
ProbeABI detectProbeAbiForExecutable(const QString &exePath);
}
