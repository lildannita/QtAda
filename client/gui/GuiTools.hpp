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
} // namespace QtAda::gui::tools
