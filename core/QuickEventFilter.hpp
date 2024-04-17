#pragma once

#include "GuiEventFilter.hpp"

namespace QtAda::core {
class QuickEventFilter : public GuiEventFilter<QQuickItem, QuickClass> {
    Q_OBJECT
public:
    QuickEventFilter(QObject *parent = nullptr) noexcept;

    void setMousePressFilter(const QObject *obj, const QEvent *event) noexcept override;
    void handleKeyEvent(const QObject *obj, const QEvent *event) noexcept override
    {
        return;
    }

signals:
    void newScriptKeyLine(const QString &line) const;
    void newPostReleaseScriptLine(const QString &line) const;

private slots:
    //! TODO: на текущий момент для QtQuick-компонентов приходится использовать
    //! "стандартную" систему `сигнал-слот`, из-за чего приходится явно прописывать слоты.
    void processSignalSlot() noexcept
    {
        delayedWatchDog_.processSignal();
    }

    void initFlickableConnection() noexcept
    {
        if (postReleaseWatchDog_.isInitForFlickableConnection()) {
            handlePostReleaseTimeout();
            return;
        }
        postReleaseWatchDog_.disconnectAll();
        postReleaseWatchDog_.connections.push_back(
            QObject::connect(postReleaseWatchDog_.flickable, SIGNAL(movementEnded()), this,
                             SLOT(callPostReleaseSlotWithEmptyArgument())));
    }

    void callPostReleaseSlotWithEmptyArgument()
    {
        callPostReleaseSlot(std::nullopt);
    }

    void callPostReleaseSlotWithIntArgument(int data)
    {
        callPostReleaseSlot(data);
    }

    void callPostReleaseSlot(std::optional<QVariant> data) noexcept
    {
        const auto result = postReleaseWatchDog_.callPostReleaseFilter(data);
        if (result.has_value() && !result->isEmpty()) {
            emit newPostReleaseScriptLine(*result);
        }
        postReleaseWatchDog_.clear();
    }

    void handlePostReleaseTimeout() noexcept
    {
        emit newPostReleaseScriptLine(filters::qMouseEventHandler(
            postReleaseWatchDog_.causedComponent, postReleaseWatchDog_.causedEvent.get()));
        postReleaseWatchDog_.clear();
    }

    void callKeyFilters() noexcept override
    {
        return;
    }

private:
    enum class PressFilterType {
        Default,
        Fake,
        PostReleaseWithTimer,
        PostReleaseWithDelayedTimer,
    };

    /*
     * Сейчас используется только для QtQuick. Проблема в том, что много 'важных для нас'
     * событий в QtQuick, в отличие от QtWidgets, происходят после Release события, из-за
     * чего мы не можем генерировать строку сразу после Release события. Поэтому используем
     * эту структуру для генерации строки не сразу после Release события, а после нужного
     * сигнала.
     */
    struct PostReleaseWatchDog {
        QTimer timer;
        const QQuickItem *causedComponent = nullptr;
        std::unique_ptr<const QMouseEvent> causedEvent = nullptr;
        Connections connections;
        std::optional<SignalMouseFilterFunction> mouseFilter = std::nullopt;
        ExtraInfoForDelayed extra;
        bool needToStartTimer = false;

        /*
         * Используется для правильной обработки изменения текущего индекса в компонентах
         * типа QQuickPathView и QQuickSwitchView. Когда мы находим эти компоненты, то
         * подключаем только сигнал movementStarted(), слот которого должен подключить
         * этот же указатель на QQuickFlickable к movementEnded(), который и приведет к
         * обработке события изменения текущего индекса.
         */
        const QQuickItem *flickable = nullptr;

        void initPostRelease(const QQuickItem *component, const QEvent *event,
                             const SignalMouseFilterFunction &filter, Connections &connections,
                             bool timerNeed, const QQuickItem *flick = nullptr) noexcept
        {
            causedEvent = utils::cloneMouseEvent(event);
            causedComponent = component;
            mouseFilter = filter;
            connections = connections;
            needToStartTimer = timerNeed;
            flickable = flick;
        }

        void startTimer() noexcept
        {
            timer.start();
        }

        bool isTimerActive() const noexcept
        {
            return timer.isActive();
        }

        void disconnectAll() noexcept
        {
            for (auto &connection : connections) {
                QObject::disconnect(connection);
            }
            connections.clear();
        }

        void clear() noexcept
        {
            timer.stop();
            disconnectAll();

            extra.clear();
            causedComponent = nullptr;
            causedEvent = nullptr;
            mouseFilter = std::nullopt;
            needToStartTimer = false;
        }

        bool isInit() const noexcept
        {
            return mouseFilter.has_value() && causedComponent != nullptr;
        }

        bool isInitForFlickableConnection()
        {
            return isInit() && flickable != nullptr;
        }

        std::optional<QString> callPostReleaseFilter(std::optional<QVariant> forExtra) noexcept
        {
            if (forExtra.has_value() && forExtra->canConvert<int>()) {
                extra.changeIndex = forExtra->toInt();
            }
            return isInit() ? std::make_optional(
                       (*mouseFilter)(causedComponent, causedEvent.get(), extra))
                            : std::nullopt;
        }
    } postReleaseWatchDog_;

    void processKeyEvent(const QString &text) noexcept override
    {
        return;
    }

    std::pair<QString, bool> callMouseFilters(const QObject *obj, const QEvent *event,
                                              bool isContinuous,
                                              bool isSpecialEvent) noexcept override;
};
} // namespace QtAda::core
