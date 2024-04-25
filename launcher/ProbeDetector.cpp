#include "ProbeDetector.hpp"

#include "LauncherLog.hpp"
#include "LauncherUtils.hpp"
#include <config.h>

#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <optional>

#ifdef HAVE_ELF_H
#include <elf.h>
#endif
#if defined(HAVE_SYS_ELF_H) && !defined(HAVE_ELF_H)
#include <sys/elf.h>
#endif

namespace QtAda::launcher::probe {
static bool libIsQtCore(const QByteArray &line)
{
    //! TODO: в будущем необходимо учесть, что в разных ОС обозначение библиотек
    //!  отличается. Сейчас ориентируемся только на Linux.
    QRegularExpression regex("^(.*)libQt(\\d+)Core\\.so(.*)$");
    QRegularExpressionMatch match = regex.match(line);
    return match.hasMatch();
}

static QString getQtCoreLibFromLdd(const QString &elfPath)
{
    const auto depLibs = QtAda::launcher::utils::getDependenciesForExecutable(elfPath);
    for (const auto &lib : depLibs) {
        if (libIsQtCore(lib)) {
            return QString::fromUtf8(lib);
        }
    }
    return QString();
}

static std::optional<std::pair<int, int>> qtVersionFromLibName(const QString &libPath)
{
    QRegularExpression regex("^(.*)\\.so\\.(\\d+)\\.(\\d+)(?:\\.(\\d+))?$");
    QRegularExpressionMatch match = regex.match(libPath);

    if (!match.hasMatch()) {
        return std::nullopt;
    }

    const auto major = match.captured(2).toInt();
    const auto minor = match.captured(3).toInt();
    return std::make_pair(major, minor);
}

static std::optional<std::pair<int, int>> qtVersionFromLibExec(const QString &libPath)
{
    /*!
     * TODO: По идее библиотеку .so можно запустить как обычный исполняемый файл,
     * но почему-то на моих машинах выдает ошибку `segmentation fault (core dumped)`
     *
     *  Код для запуска библиотеки как исполняемый файл:
     *
        QProcess proc;
        proc.setProcessChannelMode(QProcess::SeparateChannels);
        proc.setReadChannel(QProcess::StandardOutput);
        proc.start(libPath, {}, QProcess::ReadOnly);
        proc.waitForFinished();

        const QByteArray line = proc.readLine();
        const int pos = line.indexOf("Qt ");
        const QList<QByteArray> version = line.mid(pos + 2).split('.');
        if (version.size() < 3)
            return std::nullopt;

        return std::make_pair(version.at(0).toInt(), version.at(1).toInt());
    */

    // Если ldd не доступен, то на текущий момент надеемся, что доступна
    //  команда strings, возвращающая текстовые строки из исполняемого файла
    QProcess proc;
    proc.setProcessChannelMode(QProcess::SeparateChannels);
    proc.setReadChannel(QProcess::StandardOutput);
    proc.start(QStringLiteral("strings"),
               QStringList() << libPath << QStringLiteral("| grep -E \"Qt [0-9]+\\.[0-9]+\""),
               QProcess::ReadOnly);
    proc.waitForFinished();

    while (proc.canReadLine()) {
        const auto line = proc.readLine();
        if (line.isEmpty()) {
            break;
        }

        QRegularExpression regex("^Qt (\\d+)\\.(\\d+)");
        QRegularExpressionMatch match = regex.match(line);
        if (match.hasMatch()) {
            const auto major = match.captured(1).toInt();
            const auto minor = match.captured(2).toInt();
            return std::make_pair(major, minor);
        }
    }
    return std::nullopt;
}

#ifdef HAVE_ELF
template <typename ElfHeader>
static QString getArchitectureFromElfHeader(const uchar *elfData, quint64 size)
{
    if (size <= sizeof(ElfHeader))
        return QString();
    const auto *elfHeader = reinterpret_cast<const ElfHeader *>(elfData);

    switch (elfHeader->e_machine) {
    case EM_386:
        return QStringLiteral("i686");
    case EM_ARM:
        return QStringLiteral("arm");
#ifdef EM_X86_64
    case EM_X86_64:
        return QStringLiteral("x86_64");
#endif
#ifdef EM_AARCH64
    case EM_AARCH64:
        return QStringLiteral("aarch64");
#endif
    default:
        qWarning(QtAda::launcher::LauncherLog)
            << "unsupported architecture:" << elfHeader->e_machine;
        return QString();
    }
}
#endif

static QString getArchitectureFromElf(const QString &elfPath)
{
#ifdef HAVE_ELF
    QFile elf(elfPath);
    if (!elf.open(QFile::ReadOnly))
        return QString();

    const uchar *elfData = elf.map(0, elf.size());
    if (!elfData || elf.size() < EI_NIDENT)
        return QString();

    if (qstrncmp(reinterpret_cast<const char *>(elfData), ELFMAG, SELFMAG) != 0)
        return QString();

    switch (elfData[EI_CLASS]) {
    case ELFCLASS32:
        return getArchitectureFromElfHeader<Elf32_Ehdr>(elfData, elf.size());
    case ELFCLASS64:
        return getArchitectureFromElfHeader<Elf64_Ehdr>(elfData, elf.size());
    }
#else
    qWarning(launcher::LauncherLog, "сan't determine the architecture due to missing elf.h");
    Q_UNUSED(path);
#endif
    return QString();
}

ProbeABI detectProbeAbiForExecutable(const QString &elfPath) noexcept
{
    ProbeABI probe;
    if (elfPath.isEmpty()) {
        return probe;
    }

    const auto corePath = getQtCoreLibFromLdd(elfPath);
    if (corePath.isEmpty()) {
        return probe;
    }

    const QFileInfo coreFileInfo(corePath);
    if (!coreFileInfo.exists()) {
        return probe;
    }

    const auto canonicalCorePath = coreFileInfo.canonicalFilePath();
    auto qtVersion = qtVersionFromLibName(canonicalCorePath);
    if (!qtVersion.has_value()) {
        qtVersion = qtVersionFromLibExec(canonicalCorePath);
    }

    if (!qtVersion.has_value()) {
        qWarning(LauncherLog) << "сan't determine the Qt version of" << elfPath;
        return probe;
    }

    probe.setQtVersion(qtVersion.value());
    probe.setArchitecture(getArchitectureFromElf(elfPath));
    return probe;
}
} // namespace QtAda::launcher::probe
