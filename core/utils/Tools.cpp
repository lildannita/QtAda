#include "Tools.hpp"

namespace QtAda::core::tools {
std::vector<QString> cutLine(const QString &line) noexcept
{
    std::vector<int> indices;
    int index = 0;
    while ((index = line.indexOf('\n', index)) != -1) {
        if (index == 0 || line[index - 1] != '\\') {
            indices.push_back(index);
        }
        index += 1;
    }

    std::vector<QString> result;
    index = 0;
    for (const auto &position : indices) {
        result.push_back(line.mid(index, position - index));
        index = position + 1;
    }

    if (index < line.length()) {
        result.push_back(line.mid(index));
    }

    return result;
}
} // namespace QtAda::core::tools
