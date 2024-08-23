#include "ConfHandler.hpp"

#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "utils/Tools.hpp"

namespace QtAda::core {
//! TODO: В className() для классов, созданных в QML, автоматически устанавливается
//! суффикс "QMLTYPE_{число}" (это примерно то, что мы и делаем при нахождении objectPath),
//! но проблема в том, что это число - не постоянная величина (особенно это видно со
//! "специфическими" надстройками над графической оболочкой). Поэтому при запуске скриптов
//! могут быть проблемы. В связи с этим решено пока что убирать этот суффикс, если он есть.
static const auto s_qmlRegExp = QRegularExpression(
    "(?<=.)(_(QMLTYPE|QML)_\\d+)|(_QMLTYPE_\\d+_QML_\\d+)|(_QML_\\d+_QMLTYPE_\\d+)$");

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
    const QString metaObjName = QString(obj->metaObject()->className()).remove(s_qmlRegExp);
    const auto *parent = obj->parent();
    return QString("%1_%2")
        .arg(metaObjName)
        .arg(parent ? metaObjectIndexInObjectList(obj, parent->children()) : 0);
}

static QString objectId(const QObject *obj) noexcept
{
    const auto objName = obj->objectName();
    const auto *parent = obj->parent();
    return QString("%1_%2").arg(objName).arg(
        parent ? objectIndexInObjectList(obj, parent->children()) : 0);
}

static QString objectPath(const QObject *obj) noexcept
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

QAtomicPointer<ConfHandler> ConfHandler::s_instance = QAtomicPointer<ConfHandler>(nullptr);

ConfHandler::ConfHandler(const QString &confPath, QObject *parent) noexcept
    : QObject{ parent }
    , confPath_{ confPath }
{
    const auto fullDataArray = getConfArray(confPath_, true);
    if (fullDataArray.has_value()) {
        for (const auto &rawObj : *fullDataArray) {
            QJsonObject confObj = rawObj.toObject();
            pushNewConfData(confObj["id"].toString(), confObj["path"].toString(),
                            confObj["description"].toString());
        }
    }
}

ConfHandler::~ConfHandler() noexcept
{
    QJsonArray confArray;
    for (const auto &data : confData_) {
        QJsonObject confObj;
        confObj["id"] = QJsonValue(data.id);
        confObj["path"] = QJsonValue(data.path);
        confObj["description"] = QJsonValue(data.description);
        confArray.append(confObj);
    }

    QJsonDocument confDoc(confArray);
    QFile confFile(getConfAbsolutePath(confPath_, true));
    bool isOpened = confFile.open(QIODevice::WriteOnly | QIODevice::Text);
    assert(isOpened == true);
    confFile.write(confDoc.toJson(QJsonDocument::Indented));
    confFile.close();

    s_instance = QAtomicPointer<ConfHandler>(nullptr);
}

QString ConfHandler::getConfAbsolutePath(const QString &confPath, bool forRecord) noexcept
{
    // Функция лишний раз, но уже чисто для разработчика, проверяет, что файл конфигурации
    // соответствует требованиям, которые должны были быть проверены в _Settings::findErrors
    assert(!confPath.isEmpty());
    QFileInfo confInfo(confPath);
    assert(confInfo.suffix() == QStringLiteral("json"));

    bool exists = confInfo.exists();
    assert(exists || forRecord);
    assert(!exists || (confInfo.isFile() && confInfo.isReadable()));
    assert(!exists || !forRecord || confInfo.isWritable());

    return confInfo.absoluteFilePath();
}

std::optional<QJsonArray> ConfHandler::getConfArray(const QString &confPath,
                                                    bool forRecord) noexcept
{
    QFile confFile(getConfAbsolutePath(confPath, forRecord));
    if (!confFile.exists()) {
        return std::nullopt;
    }

    bool isOpened = confFile.open(QIODevice::ReadOnly | QIODevice::Text);
    assert(isOpened == true);

    const auto confFullData = confFile.readAll();
    confFile.close();

    QJsonParseError jsonError;
    QJsonDocument confDoc = QJsonDocument::fromJson(confFullData, &jsonError);
    assert(jsonError.error == QJsonParseError::NoError);
    assert(confDoc.isArray());

    return confDoc.array();
}

void ConfHandler::initialize(const QString &confPath, QObject *parent) noexcept
{
    assert(!initialized());
    s_instance = QAtomicPointer<ConfHandler>(new ConfHandler(confPath, parent));
}

bool ConfHandler::initialized() noexcept
{
    return instance();
}

ConfHandler *ConfHandler::instance() noexcept
{
    return s_instance.loadRelaxed();
}

QString ConfHandler::do_getObjectId(const QObject *obj, const QString &text) noexcept
{
    assert(obj != nullptr);

    const auto path = objectPath(obj);
    const auto className = QString(obj->metaObject()->className()).remove(s_qmlRegExp);
    auto id = tools::transliterate(text.trimmed().simplified());
    if (id.isEmpty()) {
        const auto objName = obj->objectName();
        if (!objName.isEmpty()) {
            id = objName;
        }
        else {
            id = QStringLiteral("obj");
        }
    }
    auto full_id = QStringLiteral("%1_%2").arg(id, className);

    const bool alreadyWritten = std::find_if(confData_.begin(), confData_.end(),
                                             [&full_id, &path](const ConfData &data) {
                                                 return data.id == full_id && data.path == path;
                                             })
                                != confData_.end();
    if (alreadyWritten) {
        return full_id;
    }

    int index = 1;
    while (std::find_if(confData_.begin(), confData_.end(),
                        [&full_id](const ConfData &data) { return data.id == full_id; })
           != confData_.end()) {
        full_id = QStringLiteral("%1_%2_%3").arg(id).arg(index).arg(className);
        index++;
    }
    pushNewConfData(full_id, path, text);

    return full_id;
}

void ConfHandler::pushNewConfData(const QString &id, const QString &path,
                                  const QString &description) noexcept
{
    ConfData confData;
    confData.id = id;
    confData.path = path;
    confData.description = description;
    confData_.push_back(std::move(confData));
}
} // namespace QtAda::core
