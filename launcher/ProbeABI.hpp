/*
* get/set version (minor + major)
* get/set architecture
* get/set id
*
* check valid (hasQtVersion + qtversion = 5.15 + !arch.empty())
*/

#include <QString>

namespace launcher::probe {
class ProbeABI
{
public:
    void setQtVersion(int major, int minor) noexcept;
    void setQtVersion(std::pair<int, int> version) noexcept;
    void setArchitecture(const QString architecture) noexcept;

    bool hasQtVersion() const noexcept;
    bool hasArchitecture() const noexcept;
    bool isValid() const noexcept;
private:
    struct {
        int majorVersion = -1;
        int minorQtVersion = -1;
        QString architecture;
    } info_;
};
}
