#pragma once

#include <QFrame>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QFileInfo>

QT_BEGIN_NAMESPACE
class QAbstractItemView;
QT_END_NAMESPACE

namespace QtAda::gui {
enum class AppPathCheck {
    Ok = 0,
    NoExecutable,
    NoProbe,
};
}

namespace QtAda::gui::tools {
inline QFrame *initSeparator(QWidget *parent, bool isHorizontal = false) noexcept
{
    QFrame *separator = new QFrame(parent);
    separator->setFrameShape(isHorizontal ? QFrame::HLine : QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    return separator;
}

inline void setVSpacer(QBoxLayout *layout) noexcept
{
    assert(layout != nullptr);
    layout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

inline void setHSpacer(QBoxLayout *layout) noexcept
{
    assert(layout != nullptr);
    layout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

inline bool isFileAccessible(const QFileInfo &fileInfo) noexcept
{
    return fileInfo.isFile() && fileInfo.isReadable() && fileInfo.isWritable();
}

inline bool isExistingFileAccessible(const QFileInfo &fileInfo) noexcept
{
    return fileInfo.exists() && isFileAccessible(fileInfo);
}

void deleteModels(QAbstractItemView *view) noexcept;

AppPathCheck checkProjectAppPath(const QString &path) noexcept;
QString fileNameWithoutSuffix(const QString &path) noexcept;
} // namespace QtAda::gui::tools
