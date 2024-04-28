#pragma once

#include <QFrame>

namespace QtAda::gui::tools {
inline QFrame *initSeparator(QWidget *parent, bool isHorizontal = false) noexcept
{
    QFrame *separator = new QFrame;
    separator->setFrameShape(isHorizontal ? QFrame::HLine : QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    return separator;
}
} // namespace QtAda::gui::tools
