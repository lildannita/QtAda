#pragma once

#include <QFrame>
#include <QVBoxLayout>
#include <QSpacerItem>

namespace QtAda::gui::tools {
inline QFrame *initSeparator(QWidget *parent, bool isHorizontal = false) noexcept
{
    QFrame *separator = new QFrame(parent);
    separator->setFrameShape(isHorizontal ? QFrame::HLine : QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    return separator;
}

inline void setVSpacer(QBoxLayout *layout)
{
    assert(layout != nullptr);
    layout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

inline void setHSpacer(QBoxLayout *layout)
{
    assert(layout != nullptr);
    layout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

inline QString fileNameWithoutSuffix(const QString &path)
{
    const auto lastSlashIndex = path.lastIndexOf('/');
    const auto startIndex = lastSlashIndex != -1 ? lastSlashIndex + 1 : 0;
    const auto lastDotIndex = path.lastIndexOf('.');
    const auto result = path.mid(startIndex, lastDotIndex - startIndex).trimmed();
    assert(!result.isEmpty());
    return result;
}
} // namespace QtAda::gui::tools
