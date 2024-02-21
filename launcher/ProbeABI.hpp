#pragma once

#include <QString>

namespace launcher::probe {
class ProbeABI {
public:
    void setQtVersion(int major, int minor) noexcept;
    void setQtVersion(std::pair<int, int> version) noexcept;
    void setArchitecture(const QString architecture) noexcept;

    bool hasQtVersion() const noexcept;
    bool hasArchitecture() const noexcept;
    bool isValid() const noexcept;

    QString probeDllPath() const noexcept;
    QString probeId() const noexcept;

private:
    struct {
        int majorVersion = -1;
        int minorQtVersion = -1;
        QString architecture;
    } info_;
};
} // namespace launcher::probe
