#pragma once

#include <QFrame>

namespace QtAda::inprocess {
inline QFrame *generateSeparator(QWidget *parent, bool isHorizontal = false) noexcept
{
    QFrame *separator = new QFrame;
    separator->setFrameShape(isHorizontal ? QFrame::HLine : QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    return separator;
}
}
