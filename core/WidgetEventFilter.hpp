#pragma once

#include <QtGlobal>
#include <functional>
#include <optional>
#include <variant>

#include <QObject>
#include <QEvent>
#include <QModelIndex>
#include <QTimer>

#include "ProcessedObjects.hpp"

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QString;
class QWidget;
QT_END_NAMESPACE

namespace QtAda::core::filters {
QString qMouseEventFilter(const QString &path, const QWidget *widget, const QEvent *event) noexcept;
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
using SpecialFilterFunction = std::function<QString(const QWidget *, const QEvent *)>;

class WidgetEventFilter : public QObject {
    Q_OBJECT
public:
    WidgetEventFilter(QObject *parent = nullptr) noexcept;

    QString callWidgetMouseFilters(const QWidget *widget, const QEvent *event, bool isContinuous,
                                   bool isSpecialEvent) noexcept;
    void setDelayedOrSpecificMouseEventFilter(const QWidget *widget, const QEvent *event) noexcept;

    void updateKeyWatchDog(const QWidget *widget, const QEvent *event) noexcept;
signals:
    //! TODO: Этот сигнал используется только для ввода текста, который вызывается только тогда,
    //! когда мы считаем, что редактирование текста закончилось (иначе, если бы UserEventFitler
    //! ждал scriptLine сразу после текстового QEvent, то в генерируемом скрипте был бы ввод
    //! по одной букве, что не очень хорошо). Но такую же систему, в принципе, можно попробовать
    //! и для click'овых QEvent.
    void newScriptKeyLine(const QString &line) const;

private:
    std::vector<WidgetFilterFunction> widgetMouseFilters_;
    std::vector<WidgetFilterFunction> specificWidgetMouseFilters_;
    std::map<WidgetClass, DelayedWidgetFilterFunction> delayedWidgetMouseFilters_;
    std::vector<SpecialFilterFunction> specialFilterFunctions_;

    QEvent::Type causedEventType_ = QEvent::None;
    const QEvent *causedEvent_ = nullptr;
    const QWidget *delayedWidget_ = nullptr;
    std::optional<DelayedWidgetFilterFunction> delayedMouseFilter_ = std::nullopt;
    bool needToUseDelayedMouseFilter_ = false;
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

    QTimer keyWatchDogTimer_;
    const QWidget *keyWidget_ = nullptr;
    WidgetClass keyWidgetClass_ = WidgetClass::None;
    std::vector<WidgetClass> processedTextWidgetClasses_;
    QMetaObject::Connection keyConnection_;

    void flushKeyEvent(const QString &line, const QWidget *extWidget = nullptr) noexcept;
private slots:
    void callWidgetKeyFilters() noexcept;
};
} // namespace QtAda::core
