#include "CommonFilterUtils.hpp"

#include <QObject>
#include <QMouseEvent>

namespace QtAda::core::utils {
static const std::pair<Qt::MouseButton, QLatin1String> s_mouseButtons[] = {
    { Qt::NoButton, QLatin1String("Qt::NoButton") },
    { Qt::LeftButton, QLatin1String("Qt::LeftButton") },
    { Qt::RightButton, QLatin1String("Qt::RightButton") },
    { Qt::MiddleButton, QLatin1String("Qt::MiddleButton") },
    { Qt::BackButton, QLatin1String("Qt::BackButton") },
    { Qt::ForwardButton, QLatin1String("Qt::ForwardButton") },
};

static const std::vector<std::pair<char, QString>> s_escapeReplacements
    = { { '\n', "\\n" }, { '\r', "\\r" }, { '\t', "\\t" }, { '\v', "\\v" } };

static uint metaObjectIndexInObjectList(const QObject *obj, const QObjectList &children) noexcept
{
    assert(!children.isEmpty());
    uint index = 0;
    const auto className = obj->metaObject()->className();
    for (const QObject *item : children) {
        if (item == obj) {
            return index;
        }
        if (className == item->metaObject()->className()) {
            index++;
        }
    }
    Q_UNREACHABLE();
}

static uint objectIndexInObjectList(const QObject *obj, const QObjectList &children) noexcept
{
    assert(!children.isEmpty());
    uint index = 0;
    const auto objName = obj->objectName();
    for (const QObject *item : children) {
        if (item == obj) {
            return index;
        }
        if (objName == item->objectName()) {
            index++;
        }
    }
    Q_UNREACHABLE();
}

static QString metaObjectId(const QObject *obj) noexcept
{
    const QString metaObjName = obj->metaObject()->className();
    const auto *parent = obj->parent();
    return QString("%1_%2")
        .arg(metaObjName)
        .arg(parent ? metaObjectIndexInObjectList(obj, parent->children()) : 0);
}

static QString objectId(const QObject *obj) noexcept
{
    const QString objName = obj->objectName();
    const auto *parent = obj->parent();
    return QString("%1_%2").arg(objName).arg(
        parent ? objectIndexInObjectList(obj, parent->children()) : 0);
}

QString objectPath(const QObject *obj) noexcept
{
    QStringList pathComponents;
    while (obj != nullptr) {
        QString identifier = obj->objectName().isEmpty() ? QString("c=%1").arg(metaObjectId(obj))
                                                         : QString("n=%1").arg(objectId(obj));
        pathComponents.prepend(identifier);
        obj = obj->parent();
    }
    assert(!pathComponents.isEmpty());
    return pathComponents.join('/');
}

QString escapeText(const QString &text) noexcept
{
    QString result = text;
    for (const auto &replacement : s_escapeReplacements) {
        result.replace(replacement.first, replacement.second, Qt::CaseSensitive);
    }
    return result;
}

QString mouseButtonToString(const Qt::MouseButton mouseButton) noexcept
{
    for (const auto &pair : s_mouseButtons) {
        if (pair.first == mouseButton) {
            return pair.second;
        }
    }
    return QLatin1String("<unknown>");
}

bool mouseEventCanBeFiltered(const QMouseEvent *event, bool shouldBePressEvent) noexcept
{
    const auto type = event->type();
    return event != nullptr && event->button() == Qt::LeftButton
           && (type == (shouldBePressEvent ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease)
               || type == QEvent::MouseButtonDblClick);
}
} // namespace QtAda::core::utils
