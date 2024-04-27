#pragma once

#include <rep_InprocessController_source.h>
#include <QLocalServer>
#include <QFile>

#include "Common.hpp"

namespace QtAda::inprocess {
class InprocessController : public InprocessControllerSource {
    Q_OBJECT
public:
    InprocessController(QObject *parent = nullptr) noexcept
        : InprocessControllerSource{ parent }
    {
        QFile serverFile(INIT_CONNECTION_SERVER);
        if (serverFile.exists()) {
            serverFile.remove();
        }
    }

    //! TODO: Очень страшный костыль. Дело в том, что при инициализации Probe
    //! не получается отправить сигнал через InprocessControllerReplica
    //! об успешном запуске тестируемого приложения, так как реплика очень
    //! странно ведет себя в конструкторе Probe. Что я пробовал:
    //!
    //! 1.  [НЕ РАБОТАЕТ] Самый простой способ - объявить SLOT в InprocessController.rep,
    //!     который просто бы вызывал сигнал applicationStarted(), и вызывать этот слот
    //!     в конце конструктора Probe.
    //! 2.  Объявить PROP(bool applicationStarted=false) в InprocessController.rep,
    //!     и в конце конструктора Probe установить значение в true.
    //! 2.1 [НЕ РАБОТАЕТ] Автоматически должен генерироваться сигнал об изменении значения,
    //!     но InprocessDialog не получает этот сигнал.
    //! 2.2 [НЕ РАБОТАЕТ] Установил таймер в InprocessController, который бы каждые
    //!     100-1000 мс проверял значение поля applicationStarted в InprocessController,
    //!     и когда applicationStarted == true, то отключал бы таймер и вызывал бы сигнал
    //!     о начале работы приложения. Но значение не изменяется.
    //! 3.  [РАБОТАЕТ, НО...] В Probe вызвать QTimer::singleShot(1), по окончании которого
    //!     вызывается слот из первого пункта. Это единственный способ, который сработал, но
    //!     выглядит максимально ненадежно.
    //!
    //! Текущий вариант с QLocalServer работает, но он тоже кажется не очень надежным,
    //! а также пришлось подключать библиотеку Qt::Network к проекту только для этого
    //! небольшого пинга, что не очень хорошо...

    void startInitServer() noexcept
    {
        connect(&initServer_, &QLocalServer::newConnection, this, [this] {
            initServer_.close();
            emit applicationStarted();
        });
        bool serverStarted = initServer_.listen(INIT_CONNECTION_SERVER);
        assert(serverStarted == true);
    }

public Q_SLOTS:
    // UserEventFilter -> InprocessDialog slots:
    void sendNewScriptLine(const QString &line) override
    {
        emit this->newScriptLine(line);
    }

    // UserVerificationFilter -> PropertiesWatcher slots:
    void sendNewMetaPropertyData(const QList<QVariantMap> &metaData) override
    {
        emit this->newMetaPropertyData(metaData);
    }
    void sendNewFramedRootObjectData(const QVariantMap &model,
                                     const QList<QVariantMap> &rootMetaData) override
    {
        emit this->newFramedRootObjectData(model, rootMetaData);
    }

Q_SIGNALS:
    void applicationStarted();

    // UserEventFilter -> InprocessDialog signals:
    void newScriptLine(const QString &line);

    // UserVerificationFilter -> PropertiesWatcher signals:
    void newMetaPropertyData(const QList<QVariantMap> &metaData);
    void newFramedRootObjectData(const QVariantMap &model, const QList<QVariantMap> &rootMetaData);

private:
    QLocalServer initServer_;
};
} // namespace QtAda::inprocess
