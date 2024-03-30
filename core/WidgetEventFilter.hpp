#pragma once

#include <QtGlobal>
#include <functional>
#include <optional>
#include <variant>

#include <QObject>
#include <QEvent>

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
    bool isContinuous = false;
    //! TODO: пока используем только std::optional<int>, который нужен только для определения
    //! действия над QAbstractSlider, но, возможно, позже, нужно будет менять на std::variant
    std::optional<int> changeType = std::nullopt;

    void clear() noexcept
    {
        isContinuous = false;
        changeType = std::nullopt;
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

private slots:
    void signalDetected()
    {
        needToUseFilter_ = true;
        if (connection_) {
            QObject::disconnect(connection_);
        }
    }
    void specificSignalDetected(int type)
    {
        delayedExtra_.changeType = type;
        signalDetected();
    }

private:
    std::vector<WidgetFilterFunction> filterFunctions_;
    std::map<WidgetClass, DelayedWidgetFilterFunction> delayedFilterFunctions_;

    QEvent::Type causedEventType_ = QEvent::None;
    const QEvent *causedEvent_ = nullptr;
    const QWidget *delayedWidget_ = nullptr;
    std::optional<DelayedWidgetFilterFunction> delayedFilter_ = std::nullopt;
    bool needToUseFilter_ = false;
    QMetaObject::Connection connection_;

    ExtraInfoForDelayed delayedExtra_;

    bool delayedFilterCanBeCalledForWidget(const QWidget *widget) const noexcept;
    void initDelay(const QWidget *widget, const QMouseEvent *event,
                   const DelayedWidgetFilterFunction &filter,
                   QMetaObject::Connection &connection) noexcept;
    void destroyDelay() noexcept;
    std::optional<QString> callDelayedFilter(const QWidget *widget, const QMouseEvent *event,
                                             bool isContinuous = false) noexcept;
};
} // namespace QtAda::core
