#pragma once

#include <QtGlobal>
#include <functional>
#include <optional>

#include <QObject>
#include <QEvent>

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QString;
class QWidget;
QT_END_NAMESPACE

namespace QtAda::core {
using WidgetEventFilter = std::function<QString(QWidget *, QMouseEvent *, bool)>;

class DelayedWidgetFilter : public QObject {
    Q_OBJECT
public:
    DelayedWidgetFilter(QObject *parent = nullptr)
        : QObject{ parent }
    {
    }

    void findAndSetDelayedFilter(QWidget *widget, QMouseEvent *event) noexcept;
    std::optional<QString> callDelayedFilter(QWidget *widget, QMouseEvent *event,
                                             bool isContinuous = false) noexcept;

private slots:
    void signalDetected()
    {
        needToUseFilter_ = true;
        if (connection_) {
            QObject::disconnect(connection_);
        }
    }

private:
    QWidget *delayedWidget_ = nullptr;
    QEvent *causedEvent_ = nullptr;
    QEvent::Type causedEventType_ = QEvent::None;
    std::optional<WidgetEventFilter> delayedFilter_ = std::nullopt;
    bool needToUseFilter_ = false;
    QMetaObject::Connection connection_;

    bool delayedFilterCanBeCalledForWidget(const QWidget *widget) const noexcept;
    void setDelayedFilter(QWidget *widget, QMouseEvent *event, const WidgetEventFilter &function,
                          QMetaObject::Connection &connection) noexcept;
    void destroyDelay() noexcept;
};

QString qMouseEventFilter(const QString &path, QWidget *widget, QMouseEvent *event);

// Press filters:
QString qComboBoxFilter(QWidget *widget, QMouseEvent *event, bool);
QString qButtonFilter(QWidget *widget, QMouseEvent *event, bool);
QString qCheckBoxFilter(QWidget *widget, QMouseEvent *event, bool);
} // namespace QtAda::core
