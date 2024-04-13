#pragma once

#include <QEvent>
#include <QMouseEvent>
#include <QObject>
#include <QModelIndex>
#include <optional>
#include <QTimer>
#include <QRegularExpression>
#include <QString>

#include "ProcessedObjects.hpp"
#include "utils/CommonFilters.hpp"

QT_BEGIN_NAMESPACE
class QMouseEvent;
QT_END_NAMESPACE

namespace QtAda::core {
struct ExtraInfoForDelayed final {
    enum TreeViewExtra {
        Expanded,
        Collapsed,
    };

    bool isContinuous = false;
    std::optional<int> changeType = std::nullopt;
    QModelIndex changeIndex;

    std::vector<int> collectedIndexes;

    void clear() noexcept
    {
        isContinuous = false;
        changeType = std::nullopt;
        changeIndex = QModelIndex();
        collectedIndexes.clear();
    }
};

struct MouseEventInfo final {
    bool isContinuous = false;
    bool isSpecialEvent = false;
    bool duplicateMouseEvent = false;
    QString objPath = QString();
};

class GuiEventFilterBase : public QObject {
    Q_OBJECT
public:
    GuiEventFilterBase(QObject *parent = nullptr)
        : QObject{ parent }
    {
    }

    virtual ~GuiEventFilterBase() = default;

    virtual void setMousePressFilter(const QObject *obj, const QEvent *event) noexcept = 0;
    QString handleMouseEvent(const QObject *obj, const QEvent *event,
                             const MouseEventInfo &info) noexcept
    {
        auto scriptLine = callMouseFilters(obj, event, info.isContinuous, info.isSpecialEvent);
        if (scriptLine.isEmpty()) {
            scriptLine = filters::qMouseEventHandler(obj, event, info.objPath);
        }
        else if (info.duplicateMouseEvent) {
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

signals:
    // Для MouseEvent имеется потребность "откладывать" сгенерированную строку, поэтому используем
    // сигнал только для KeyEvent, так как для этого события сгенерированная строка является
    // итоговой.
    void newKeyScriptLine(const QString &line) const;

protected:
    virtual void processKeyEvent(const QString &text) noexcept = 0;
    virtual QString callMouseFilters(const QObject *obj, const QEvent *event, bool isContinuous,
                                     bool isSpecialEvent) noexcept
        = 0;

protected slots:
    virtual void callKeyFilters() noexcept = 0;
};

template <typename GuiComponent, typename EnumType>
class GuiEventFilter : public GuiEventFilterBase {
protected:
    using FilterFunction = std::function<QString(const GuiComponent *, const QEvent *)>;
    using MouseFilterFunction = std::function<QString(const GuiComponent *, const QMouseEvent *)>;
    using DelayedMouseFilterFunction = std::function<QString(
        const GuiComponent *, const QMouseEvent *, const ExtraInfoForDelayed &)>;

    using Connections = std::vector<QMetaObject::Connection>;

    GuiEventFilter(QObject *parent = nullptr)
        : GuiEventFilterBase{ parent }
    {
        CHECK_GUI_CLASS(GuiComponent);
        CHECK_GUI_ENUM(EnumType);
    }

    std::vector<MouseFilterFunction> mouseFilters_;
    std::vector<MouseFilterFunction> specificMouseFilters_;
    std::map<EnumType, DelayedMouseFilterFunction> delayedMouseFilters_;
    std::vector<FilterFunction> specialFilters_;

    struct DelayedData {
        DelayedData()
        {
            bool test = false;
        }

        const GuiComponent *causedComponent = nullptr;
        QEvent::Type causedEventType = QEvent::None;
        const QEvent *causedEvent = nullptr;
        std::optional<DelayedMouseFilterFunction> mouseFilter = std::nullopt;
        bool signalDetected = false;
        Connections connections;
        ExtraInfoForDelayed extra;
        QString specificResult;

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

        bool connectionIsInit(std::optional<Connections> extConnections
                              = std::nullopt) const noexcept
        {
            for (auto &connection : extConnections.has_value() ? *extConnections : connections) {
                if (connection) {
                    return true;
                }
            }
            return false;
        }

        void initDelay(const GuiComponent *component, const QEvent *event,
                       const DelayedMouseFilterFunction &filter, Connections &connections) noexcept
        {
            causedEvent = event;
            causedEventType = event->type();
            causedComponent = component;
            mouseFilter = filter;
            connections = connections;
        }

        void initSpecific(const GuiComponent *component, const QEvent *event,
                          const QString &result) noexcept
        {
            causedEvent = event;
            causedEventType = event->type();
            causedComponent = component;
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
            return signalDetected && !connectionIsInit() && mouseFilter.has_value()
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
    } delayedData_;

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
