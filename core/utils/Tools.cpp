#include "Tools.hpp"

#include <QGuiApplication>
#include <QObject>
#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaEnum>
#include <QByteArray>
#include <optional>

#include <QCursor>
#include <QIcon>
#include <QLocale>
#include <QPalette>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QTimeZone>
#include <QEasingCurve>
#include <QJsonObject>
#include <QJsonArray>

Q_DECLARE_METATYPE(const QMetaObject *)
Q_DECLARE_METATYPE(const QMatrix4x4 *)
Q_DECLARE_METATYPE(QTimeZone)

namespace QtAda::core::tools {
class ProtectedObjectSample final : public QObject {
public:
    using QObject::staticQtMetaObject;
};

static bool isVariantEnum(const QVariant &value)
{
    if (!value.isValid()) {
        return false;
    }
    const QMetaType mt(value.userType());
    return mt.isValid() && mt.flags() & QMetaType::IsEnumeration;
}

static const QMetaObject *metaObjectForClass(const QByteArray &className)
{
    if (className.isEmpty()) {
        return nullptr;
    }
    auto *metaObject = QMetaType::metaObjectForType(QMetaType::type(className));
    if (metaObject != nullptr) {
        return metaObject;
    }
    // Если не получили мета-объект по обычному имени класса, то пробуем
    // версию с указателем
    metaObject = QMetaType::metaObjectForType(QMetaType::type(className + '*'));
    return metaObject;
}

static std::optional<QMetaEnum> getMetaEnum(const QVariant &value, const char *сanonicalTypeName,
                                            const QMetaObject *сanonicalMetaObject = nullptr)
{
    QByteArray typeName(сanonicalTypeName);
    if (typeName.isEmpty()) {
        typeName = value.typeName();
    }

    // Нужно разделить имя класса и имя перечисления
    QByteArray className;
    QByteArray enumTypeName(typeName);
    const auto lastSeparatorIndex = enumTypeName.lastIndexOf("::");
    if (lastSeparatorIndex >= 0) {
        className = enumTypeName.left(lastSeparatorIndex);
        enumTypeName = enumTypeName.mid(lastSeparatorIndex + 2);
    }

    // Получаем индекс перечисления "каноничным" путем
    const auto *metaObject = &ProtectedObjectSample::staticQtMetaObject;
    auto enumIndex = metaObject->indexOfEnumerator(enumTypeName);
    if (enumIndex < 0 && сanonicalMetaObject != nullptr) {
        // Если индекс не получили, но у нас есть "исходный" мета-объект,
        // пробуем получить через него
        metaObject = сanonicalMetaObject;
        enumIndex = metaObject->indexOfEnumerator(enumTypeName);
    }
    if (enumIndex < 0 && (metaObject = QMetaType::metaObjectForType(QMetaType::type(typeName)))) {
        // Если индекс не получили, но смогли получить мета-объект через его тип,
        // пробуем получить через него
        enumIndex = metaObject->indexOfEnumerator(enumTypeName);
    }
    if (enumIndex < 0 && (metaObject = metaObjectForClass(className))) {
        // Если индекс не получили, но смогли получить мета-объект через его имя класса,
        // пробуем получить через него
        enumIndex = metaObject->indexOfEnumerator(enumTypeName);
    }

    if (enumIndex < 0 && сanonicalMetaObject != nullptr) {
        // Если так и не получилось, то пытаемся получить из другого
        // пространства имен, вызывая эту же функцию рекурсивно
        QByteArray classParentName(сanonicalMetaObject->className());
        const auto separatorIndex = classParentName.lastIndexOf("::");
        if (separatorIndex > 0) {
            classParentName = classParentName.left(separatorIndex + 2) + typeName;
            return getMetaEnum(value, classParentName, nullptr);
        }
    }

    if (enumIndex < 0) {
        return std::nullopt;
    }

    return metaObject->enumerator(enumIndex);
}

static int metaEnumToInt(const QVariant &value, const QMetaEnum &metaEnum)
{
    if (metaEnum.isFlag() && QMetaType::sizeOf(value.userType()) == sizeof(int)) {
        return value.constData() ? *static_cast<const int *>(value.constData()) : 0;
    }
    return value.toInt();
}

static QString metaEnumToString(const QVariant &value, const char *typeName = nullptr,
                                const QMetaObject *metaObject = nullptr)
{
    const auto metaEnum = getMetaEnum(value, typeName, metaObject);
    if (!metaEnum.has_value() || !metaEnum->isValid()) {
        return QString();
    }

    if (metaEnum->isFlag()) {
        return QString::fromUtf8(metaEnum->valueToKeys(metaEnumToInt(value, metaEnum.value())));
    }
    else {
        return QString::fromUtf8(metaEnum->valueToKey(metaEnumToInt(value, metaEnum.value())));
    }

    //! TODO: В будущем нужно будет перехватывать и регистрировать пользовательские типы
    //! и искать QMetaEnum среди них.
}

static QString matrix4x4ToString(const QMatrix4x4 &matrix)
{
    QStringList rows;
    rows.reserve(4);
    for (int i = 0; i < 4; ++i) {
        QStringList columns;
        columns.reserve(4);
        for (int j = 0; j < 4; ++j) {
            columns.push_back(QString::number(matrix(i, j)));
        }
        rows.push_back(columns.join(QStringLiteral(" ")));
    }
    return '[' + rows.join(QStringLiteral(", ")) + ']';
}

static QString matrix4x4ToString(const QMatrix4x4 *matrix)
{
    if (matrix) {
        return matrix4x4ToString(*matrix);
    }
    return QStringLiteral("<null>");
}

template <typename T> static QString vectorToString(const T &vector, int dimension)
{
    QStringList v;
    for (int i = 0; i < dimension; ++i) {
        v.push_back(QString::number(vector[i]));
    }
    return '[' + v.join(QStringLiteral(", ")) + ']';
}

static QString variantValueToString(const QVariant &value)
{
    switch (value.type()) {
#ifndef QT_NO_CURSOR
    case QVariant::Cursor: {
        const QCursor cursor = value.value<QCursor>();
        return metaEnumToString(QVariant::fromValue<int>(cursor.shape()), "Qt::CursorShape");
    }
#endif
    case QVariant::Icon: {
        const QIcon icon = value.value<QIcon>();
        if (icon.isNull()) {
            return QStringLiteral("<no icon>");
        }
        const auto sizes = icon.availableSizes();
        QStringList stringSizeList;
        stringSizeList.reserve(sizes.size());
        for (QSize size : sizes) {
            stringSizeList.push_back(variantValueToString(size));
        }
        return stringSizeList.join(QStringLiteral(", "));
    }
    case QVariant::Line: {
        const auto line = value.toLine();
        return QStringLiteral("%1, %2 → %3, %4")
            .arg(line.x1())
            .arg(line.y1())
            .arg(line.x2())
            .arg(line.y2());
    }
    case QVariant::LineF: {
        const auto line = value.toLineF();
        return QStringLiteral("%1, %2 → %3, %4")
            .arg(line.x1())
            .arg(line.y1())
            .arg(line.x2())
            .arg(line.y2());
    }
    case QVariant::Locale: {
        return value.toLocale().name();
    }
    case QVariant::Point: {
        const auto point = value.toPoint();
        return QStringLiteral("%1, %2").arg(point.x()).arg(point.y());
    }
    case QVariant::PointF: {
        const auto point = value.toPointF();
        return QStringLiteral("%1, %2").arg(point.x()).arg(point.y());
    }
    case QVariant::Rect: {
        const auto rect = value.toRect();
        return QStringLiteral("%1, %2 %3 x %4")
            .arg(rect.x())
            .arg(rect.y())
            .arg(rect.width())
            .arg(rect.height());
    }
    case QVariant::RectF: {
        const auto rect = value.toRectF();
        return QStringLiteral("%1, %2 %3 x %4")
            .arg(rect.x())
            .arg(rect.y())
            .arg(rect.width())
            .arg(rect.height());
    }
    case QVariant::Palette: {
        const QPalette pal = value.value<QPalette>();
        if (pal == qApp->palette()) {
            return QStringLiteral("<inherited>");
        }
        return QStringLiteral("<custom>");
    }
    case QVariant::Size: {
        const auto size = value.toSize();
        return QStringLiteral("%1 x %2").arg(size.width()).arg(size.height());
    }
    case QVariant::SizeF: {
        const auto size = value.toSizeF();
        return QStringLiteral("%1 x %2").arg(size.width()).arg(size.height());
    }
    case QVariant::StringList: {
        const auto stringList = value.toStringList();
        if (stringList.isEmpty()) {
            return QStringLiteral("<empty>");
        }
        if (stringList.size() == 1) {
            return stringList.at(0);
        }
        //! TODO: Может нужно все-таки перечислять все элементы в списке
        return QStringLiteral("<%1 entries>").arg(stringList.size());
    }
    case QVariant::Transform: {
        const QTransform t = value.value<QTransform>();
        return QStringLiteral("[%1 %2 %3, %4 %5 %6, %7 %8 %9]")
            .arg(t.m11())
            .arg(t.m12())
            .arg(t.m13())
            .arg(t.m21())
            .arg(t.m22())
            .arg(t.m23())
            .arg(t.m31())
            .arg(t.m32())
            .arg(t.m33());
    }
    default:
        break;
    }

    // types with dynamic type ids
    if (value.userType() == qMetaTypeId<uchar>()) {
        const auto v = value.value<uchar>();
        return QString::number(v) + QLatin1String(" '") + QChar(v) + QLatin1Char('\'');
    }

    if (value.userType() == qMetaTypeId<const QMetaObject *>()) {
        const auto metaObject = value.value<const QMetaObject *>();
        if (metaObject == nullptr) {
            return QStringLiteral("0x0");
        }
        return metaObject->className();
    }

    if (value.userType() == qMetaTypeId<QMatrix4x4>()) {
        return matrix4x4ToString(value.value<QMatrix4x4>());
    }
    if (value.userType() == qMetaTypeId<const QMatrix4x4 *>()) {
        return matrix4x4ToString(value.value<const QMatrix4x4 *>());
    }

    if (value.userType() == qMetaTypeId<QVector2D>()) {
        return vectorToString(value.value<QVector2D>(), 2);
    }
    if (value.userType() == qMetaTypeId<QVector3D>()) {
        return vectorToString(value.value<QVector3D>(), 3);
    }
    if (value.userType() == qMetaTypeId<QVector4D>()) {
        return vectorToString(value.value<QVector4D>(), 4);
    }

    if (value.userType() == qMetaTypeId<QTimeZone>()) {
        return value.value<QTimeZone>().id();
    }

    if (value.userType() == qMetaTypeId<QSet<QByteArray>>()) {
        const QSet<QByteArray> set = value.value<QSet<QByteArray>>();
        QStringList byteList;
        byteList.reserve(set.size());
        for (const QByteArray &byte : set) {
            byteList.push_back(QString::fromUtf8(byte));
        }
        return byteList.join(QStringLiteral(", "));
    }

    if (value.userType() == qMetaTypeId<QEasingCurve>()) {
        const auto ec = value.toEasingCurve();
        return metaEnumToString(QVariant::fromValue<QEasingCurve::Type>(ec.type()));
    }

    // Если value - enum, то metaEnumToString вернет не пустую строку
    const QString enumStr = metaEnumToString(value);
    if (!enumStr.isEmpty()) {
        return enumStr;
    }

    //! TODO: Обходной путь https://bugreports.qt.io/browse/QTBUG-73437.
    //! Исправить, когда баг починят
    if (value.userType() == qMetaTypeId<QJsonObject>()) {
        const auto size = value.value<QJsonObject>().size();
        if (size == 0) {
            return QStringLiteral("<empty>");
        }
        else {
            return QStringLiteral("<%1 entries>").arg(size);
        }
    }

    if (value.userType() == qMetaTypeId<QJsonArray>()) {
        const auto size = value.value<QJsonArray>().size();
        if (size == 0) {
            return QStringLiteral("<empty>");
        }
        else {
            return QStringLiteral("<%1 entries>").arg(size);
        }
    }

    if (value.userType() == qMetaTypeId<QJsonValue>()) {
        QJsonValue v = value.value<QJsonValue>();
        if (v.isBool()) {
            return v.toBool() ? QStringLiteral("true") : QStringLiteral("false");
        }
        else if (v.isDouble()) {
            return QString::number(v.toDouble());
        }
        else if (v.isNull()) {
            return QStringLiteral("null");
        }
        else if (v.isArray()) {
            int size = v.toArray().size();
            if (size == 0) {
                return QStringLiteral("<empty>");
            }
            else {
                return QStringLiteral("<%1 entries>").arg(size);
            }
        }
        else if (v.isObject()) {
            int size = v.toObject().size();
            if (size == 0) {
                return QStringLiteral("<empty>");
            }
            else {
                return QStringLiteral("<%1 entries>").arg(size);
            }
        }
        else if (v.isString()) {
            return v.toString();
        }
        else {
            return QStringLiteral("undefined");
        }
    }

    if (value.canConvert<QVariantList>() && !value.canConvert<QString>()) {
        QSequentialIterable it = value.value<QSequentialIterable>();
        if (it.size() == 0) {
            return QStringLiteral("<empty>");
        }
        else {
            return QStringLiteral("<%1 entries>").arg(it.size());
        }
    }
    if (value.canConvert<QVariantHash>()) {
        auto it = value.value<QAssociativeIterable>();
        if (it.size() == 0) {
            return QStringLiteral("<empty>");
        }
        else {
            return QStringLiteral("<%1 entries>").arg(it.size());
        }
    }

    if (isVariantEnum(value)) {
        return QString::number(value.toInt());
    }

    return value.toString();
}

QString metaPropertyValueToString(const QObject *obj, const QMetaProperty &property) noexcept
{
    const auto propertyValue = obj->property(property.name());
    if (property.isValid()) {
        const auto enumValue
            = metaEnumToString(propertyValue, property.typeName(), obj->metaObject());
        if (!enumValue.isEmpty()) {
            return enumValue;
        }
    }
    return variantValueToString(std::move(propertyValue));
}
} // namespace QtAda::core::tools
