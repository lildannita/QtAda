#pragma once

#include <QEvent>
#include <QMouseEvent>
#include <QObject>
#include <QModelIndex>
#include <optional>
#include <QTimer>
#include <QRegularExpression>
#include <QString>

#include "GenerationSettings.hpp"
#include "ProcessedObjects.hpp"
#include "utils/CommonFilters.hpp"

QT_BEGIN_NAMESPACE
class QMouseEvent;
QT_END_NAMESPACE

namespace QtAda::core {
struct MouseEventInfo final {
    bool isContinuous = false;
    QString objPath = QString();
};

struct ExtraInfoForDelayed final {
    enum TreeViewExtra {
        Expanded,
        Collapsed,
    };

    const GenerationSettings generationSettings;

    bool isContinuous = false;
    std::optional<int> changeType = std::nullopt;
    QModelIndex changeModelIndex;
    std::optional<int> changeIndex;
    std::vector<int> collectedIndexes;

    ExtraInfoForDelayed(const GenerationSettings &settings)
        : generationSettings{ settings }
    {
    }

    void clear() noexcept
    {
        isContinuous = false;
        changeType = std::nullopt;
        changeModelIndex = QModelIndex();
        changeIndex = std::nullopt;
        collectedIndexes.clear();
    }
};

class GuiEventFilterBase : public QObject {
    Q_OBJECT
public:
    GuiEventFilterBase(const GenerationSettings &settings, QObject *parent = nullptr)
        : QObject{ parent }
        , generationSettings_{ settings }
    {
        assert(generationSettings_.isInit());
    }

    virtual ~GuiEventFilterBase() = default;

    virtual void setMousePressFilter(const QObject *obj, const QEvent *event) noexcept = 0;
    std::optional<QString> handleMouseEvent(const QObject *obj, const QEvent *event,
                                            const MouseEventInfo &info) noexcept
    {
        auto [scriptLine, skip] = callMouseFilters(obj, event, info.isContinuous);
        if (skip) {
            // skip нужен для пропуска генерации в случае ожидания генерации от PostRelease
            return std::nullopt;
        }

        if (scriptLine.isEmpty()) {
            scriptLine = filters::qMouseEventHandler(obj, event, info.objPath);
        }
        else if (generationSettings_.duplicateMouseEvent) {
            static QRegularExpression s_regex("mouse(Dbl)?Click");
            if (!s_regex.match(scriptLine).hasMatch()) {
                scriptLine += QStringLiteral("// %1").arg(
                    filters::qMouseEventHandler(obj, event, info.objPath));
            }
        }
        assert(!scriptLine.isEmpty());
        return scriptLine;
    }

    virtual void handleKeyEvent(const QObject *obj, const QEvent *event) noexcept = 0;
    virtual QString handleCloseEvent(const QObject *obj, const QEvent *event) noexcept = 0;

signals:
    // Для MouseEvent имеется потребность "откладывать" сгенерированную строку, поэтому используем
    // сигнал только для KeyEvent, так как для этого события сгенерированная строка является
    // итоговой.
    void newKeyScriptLine(const QString &line) const;

protected:
    const GenerationSettings generationSettings_;

    virtual void processKeyEvent(const QString &text) noexcept = 0;
    virtual std::pair<QString, bool> callMouseFilters(const QObject *obj, const QEvent *event,
                                                      bool isContinuous) noexcept
        = 0;

protected slots:
    virtual void callKeyFilters() noexcept = 0;
};

template <typename GuiComponent, typename EnumType>
class GuiEventFilter : public GuiEventFilterBase {
protected:
    using MouseFilterFunction = std::function<QString(const GuiComponent *, const QMouseEvent *,
                                                      const GenerationSettings &)>;
    using SignalMouseFilterFunction = std::function<QString(
        const GuiComponent *, const QMouseEvent *, const ExtraInfoForDelayed &)>;

    using Connections = std::vector<QMetaObject::Connection>;

    GuiEventFilter(const GenerationSettings &settings, QObject *parent = nullptr)
        : GuiEventFilterBase{ settings, parent }
        , delayedWatchDog_{ settings }
    {
        CHECK_GUI_CLASS(GuiComponent);
        CHECK_GUI_ENUM(EnumType);

        auto &keyWatchDogTimer = keyWatchDog_.timer;
        keyWatchDogTimer.setInterval(5000);
        keyWatchDogTimer.setSingleShot(true);
        connect(&keyWatchDogTimer, &QTimer::timeout, this, &GuiEventFilter::callKeyFilters);
    }

    std::vector<MouseFilterFunction> mouseFilters_;
    std::map<EnumType, SignalMouseFilterFunction> signalMouseFilters_;

    struct DelayedWatchDog {
        const GuiComponent *causedComponent = nullptr;
        QEvent::Type causedEventType = QEvent::None;
        const QEvent *causedEvent = nullptr;
        std::optional<SignalMouseFilterFunction> mouseFilter = std::nullopt;
        bool signalDetected = false;
        bool isFake = false;
        Connections connections;
        ExtraInfoForDelayed extra;
        QString specificResult;

        DelayedWatchDog(const GenerationSettings &settings)
            : extra{ settings }
        {
        }

        void disconnectAll() noexcept
        {
            for (auto &connection : connections) {
                QObject::disconnect(connection);
            }
            connections.clear();
        }

        void processSignal(bool needToDisconnect = true) noexcept
        {
            signalDetected = true;
            if (needToDisconnect) {
                disconnectAll();
            }
        }

        bool connectionIsInit() const noexcept
        {
            return utils::connectionIsInit(connections);
        }

        void init(const GuiComponent *component, const QEvent *event)
        {
            causedEvent = event;
            causedEventType = event->type();
            causedComponent = component;
        }

        void initDelay(const GuiComponent *component, const QEvent *event,
                       const SignalMouseFilterFunction &filter, Connections &connections) noexcept
        {
            init(component, event);
            mouseFilter = filter;
            connections = connections;
        }

        // Пока что используется только для QQuickSpinBox.
        void initFakeDelay(const GuiComponent *component, const QEvent *event,
                           const SignalMouseFilterFunction &filter) noexcept
        {
            init(component, event);
            mouseFilter = filter;
            isFake = true;
        }

        void initSpecific(const GuiComponent *component, const QEvent *event,
                          const QString &result) noexcept
        {
            init(component, event);
            specificResult = std::move(result);
        }

        void clear() noexcept
        {
            specificResult.clear();

            causedEventType = QEvent::None;
            causedComponent = nullptr;
            mouseFilter = std::nullopt;
            extra.clear();
            signalDetected = false;
            isFake = false;

            disconnectAll();
        }

        bool specificResultCanBeShown(const GuiComponent *component) const noexcept
        {
            auto *componentParent = qobject_cast<const GuiComponent *>(component->parent());
            return causedComponent != nullptr
                   && (component == causedComponent || componentParent == causedComponent)
                   && !specificResult.isEmpty();
        }

        bool delayedFilterCanBeCalled(const GuiComponent *component) const noexcept
        {
            return (isFake || signalDetected) && !connectionIsInit() && mouseFilter.has_value()
                   && causedComponent != nullptr && causedComponent == component;
        }

        std::optional<QString> callDelayedFilter(const GuiComponent *component,
                                                 const QMouseEvent *event,
                                                 bool isContinuous) noexcept
        {
            disconnectAll();
            extra.isContinuous = isContinuous;
            bool callable = delayedFilterCanBeCalled(component);
            return callable ? std::make_optional((*mouseFilter)(component, event, extra))
                            : std::nullopt;
        }
    } delayedWatchDog_;

    struct KeyWatchDog {
        QTimer timer;
        const GuiComponent *component = nullptr;
        EnumType componentClass = EnumType::None;
        QMetaObject::Connection connection;

        void clear() noexcept
        {
            timer.stop();
            component = nullptr;
            componentClass = EnumType::None;
            if (connection) {
                QObject::disconnect(connection);
            }
        }
    } keyWatchDog_;

    void flushKeyEvent(const QString &line) const noexcept
    {
        assert(!line.isEmpty());
        emit newKeyScriptLine(line);
    }
};
} // namespace QtAda::core
