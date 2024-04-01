#pragma once

#include <QtGlobal>
#include <functional>
#include <optional>
#include <variant>

#include <QObject>
#include <QEvent>
#include <QModelIndex>

#include "ProcessedObjects.hpp"

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QString;
class QWidget;
QT_END_NAMESPACE

namespace QtAda::core::filters {
QString qMouseEventFilter(const QString &path, const QWidget *widget,
                          const QMouseEvent *event) noexcept;
}

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

using WidgetFilterFunction = std::function<QString(const QWidget *, const QMouseEvent *)>;
using DelayedWidgetFilterFunction
    = std::function<QString(const QWidget *, const QMouseEvent *, const ExtraInfoForDelayed &)>;

class WidgetEventFilter : public QObject {
    Q_OBJECT
public:
    WidgetEventFilter(QObject *parent = nullptr) noexcept;

    QString callWidgetFilters(const QWidget *widget, const QMouseEvent *event,
                              bool isContinuous) noexcept;
    void findAndSetDelayedFilter(const QWidget *widget, const QMouseEvent *event) noexcept;

private:
    std::vector<WidgetFilterFunction> filterFunctions_;
    std::vector<WidgetFilterFunction> specificFilterFunctions_;
    std::map<WidgetClass, DelayedWidgetFilterFunction> delayedFilterFunctions_;

    QEvent::Type causedEventType_ = QEvent::None;
    const QEvent *causedEvent_ = nullptr;
    const QWidget *delayedWidget_ = nullptr;
    std::optional<DelayedWidgetFilterFunction> delayedFilter_ = std::nullopt;
    bool needToUseDelayedFilter_ = false;
    std::vector<QMetaObject::Connection> connections_;
    ExtraInfoForDelayed delayedExtra_;

    QString specificResult_;

    void signalDetected(bool needToDisconnect = true) noexcept;
    bool connectionIsInit(std::optional<std::vector<QMetaObject::Connection>> connections
                          = std::nullopt) const noexcept;
    void disconnectAll() noexcept;

    bool specificResultCanBeShown(const QWidget *widget) const noexcept;
    bool delayedFilterCanBeCalledForWidget(const QWidget *widget) const noexcept;
    void initDelay(const QWidget *widget, const QMouseEvent *event,
                   const DelayedWidgetFilterFunction &filter,
                   std::vector<QMetaObject::Connection> &connections) noexcept;
    void initSpecific(const QWidget *widget, const QEvent *event, const QString &result) noexcept;
    void destroyDelay() noexcept;

    std::optional<QString> callDelayedFilter(const QWidget *widget, const QMouseEvent *event,
                                             bool isContinuous = false) noexcept;
};
} // namespace QtAda::core
