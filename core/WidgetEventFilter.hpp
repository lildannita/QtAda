#pragma once

#include <QtGlobal>
#include <functional>
#include <optional>

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
using WidgetFilterFunction = std::function<QString(const QWidget *, const QMouseEvent *, bool)>;
class WidgetEventFilter : public QObject {
    Q_OBJECT
public:
    WidgetEventFilter(QObject *parent = nullptr) noexcept;

    QString callWidgetFilters(const QWidget *widget, const QMouseEvent *event,
                              bool isDelayed) const noexcept;
    void findAndSetDelayedFilter(const QWidget *widget, const QMouseEvent *event) noexcept;

private slots:
    void signalDetected()
    {
        needToUseFilter_ = true;
        if (connection_) {
            QObject::disconnect(connection_);
        }
    }

private:
    std::vector<WidgetFilterFunction> filterFunctions_;
    std::map<WidgetClass, WidgetFilterFunction> delayedFilterFunctions_;

    QEvent::Type causedEventType_ = QEvent::None;
    const QEvent *causedEvent_ = nullptr;
    const QWidget *delayedWidget_ = nullptr;
    std::optional<WidgetFilterFunction> delayedFilter_ = std::nullopt;
    bool needToUseFilter_ = false;
    QMetaObject::Connection connection_;

    bool delayedFilterCanBeCalledForWidget(const QWidget *widget) const noexcept;
    void initDelay(const QWidget *widget, const QMouseEvent *event,
                   const WidgetFilterFunction &filter,
                   QMetaObject::Connection &connection) noexcept;
    void destroyDelay() noexcept;
    std::optional<QString> callDelayedFilter(const QWidget *widget, const QMouseEvent *event,
                                             bool isContinuous = false) const noexcept;
};
} // namespace QtAda::core
