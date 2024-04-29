#pragma once

#include <QFrame>

namespace QtAda::inprocess::tools {
inline QFrame *initSeparator(QWidget *parent, bool isHorizontal = false) noexcept
{
    QFrame *separator = new QFrame(parent);
    separator->setFrameShape(isHorizontal ? QFrame::HLine : QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    return separator;
}
} // namespace QtAda::inprocess::tools
