#pragma once

#include <QObject>
#include <QString>
#include <optional>

#include "ProcessedObjects.hpp"

namespace QtAda::core {
class ConfHandler final : public QObject {
    Q_OBJECT
public:
    explicit ConfHandler(const QString &confPath, QObject *parent) noexcept;
    ~ConfHandler() noexcept;

    static void initialize(const QString &confPath, QObject *parent) noexcept;
    static std::optional<QJsonArray> getConfArray(const QString &confPath, bool forRecord) noexcept;

    static QString getObjectPath(const QObject *obj) noexcept;
    static QString getObjectIdWithSpecifiedPath(const QObject *obj, const QString &path,
                                                const QString &text = QString()) noexcept
    {
        return instance()->internalIdGetter(obj, path, text);
    }
    static QString getObjectId(const QObject *obj, const QString &text = QString()) noexcept
    {
        assert(initialized() == true);
        return instance()->do_getObjectId(obj, text);
    }

    template <typename GuiComponent>
    static QString getObjectId(const GuiComponent *component,
                               const QString &text = QString()) noexcept
    {
        CHECK_GUI_CLASS(GuiComponent);
        auto *obj = qobject_cast<const QObject *>(component);
        assert(obj != nullptr);
        return getObjectId(obj, text);
    }

private:
    static QAtomicPointer<ConfHandler> s_instance;
    const QString confPath_;

    struct ConfData final {
        QString id;
        QString path;
        QString description;
    };
    std::vector<ConfData> confData_;

    static bool initialized() noexcept;
    static ConfHandler *instance() noexcept;

    static QString getConfAbsolutePath(const QString &confPath, bool forRecord) noexcept;
    void pushNewConfData(const QString &id, const QString &path,
                         const QString &description) noexcept;

    QString internalIdGetter(const QObject *obj, const QString &path, const QString &text) noexcept;
    QString do_getObjectId(const QObject *obj, const QString &text) noexcept
    {
        return internalIdGetter(obj, getObjectPath(obj), text);
    }
};
} // namespace QtAda::core
