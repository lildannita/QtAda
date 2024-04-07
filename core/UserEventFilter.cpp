#include "UserEventFilter.hpp"

#include <QApplication>
#include <QQuickItem>
#include <QWidget>
#include <QRegularExpression>

#include "utils/FilterUtils.hpp"

//! TODO: remove
#include <iostream>

namespace QtAda::core {
static constexpr qint64 EVENT_TIME_DIFF_MS = 100;

bool UserEventFilter::LastMouseEvent::registerEvent(const QString &path,
                                                    const QEvent *event) noexcept
{
    auto *mouseEvent = static_cast<const QMouseEvent *>(event);
    if (mouseEvent == nullptr) {
        return false;
    }

    const auto now = QDateTime::currentDateTime();
    if (mouseEvent->type() == type && std::llabs(now.msecsTo(timestamp)) < EVENT_TIME_DIFF_MS
        && mouseEvent->buttons() == buttons) {
        if (mouseEvent->globalPos() == globalPos && objectPath == path) {
            return false;
        }
        const auto parentPath = objectPath.left(objectPath.lastIndexOf('/'));
        if (parentPath == path) {
            objectPath = path;
            return false;
        }
    }

    type = mouseEvent->type();
    timestamp = now;
    globalPos = mouseEvent->globalPos();
    buttons = mouseEvent->buttons();
    objectPath = path;

    return true;
}

bool UserEventFilter::LastMouseEvent::isContinuous(const LastMouseEvent &pressEvent) const noexcept
{
    return type == QEvent::MouseButtonRelease
           && (pressEvent.type == QEvent::MouseButtonPress
               || pressEvent.type == QEvent::MouseButtonDblClick)
           && std::llabs(timestamp.msecsTo(pressEvent.timestamp)) > EVENT_TIME_DIFF_MS
           && objectPath == pressEvent.objectPath;
}

UserEventFilter::UserEventFilter(QObject *parent) noexcept
    : QObject{ parent }
{
    doubleClickTimer_.setSingleShot(true);
    connect(&doubleClickTimer_, &QTimer::timeout, this, [this]() {
        flushScriptLines(delayedScriptLines_);
        delayedScriptLines_.clear();
    });

    //! TODO: убрать
    connect(this, &UserEventFilter::newScriptLine, this,
            [](const QString &line) { std::cout << line.toStdString() << std::endl; });
}

void UserEventFilter::flushScriptLines(const QStringList &list) const noexcept
{
    for (const auto &line : list) {
        assert(!line.isEmpty());
        emit newScriptLine(line);
    }
}

QStringList UserEventFilter::handleMouseEvent(const QString &objPath, const QWidget *widget,
                                              const QEvent *event, bool isSpecialEvent) noexcept
{
    auto scriptLine = widgetFilter_.callWidgetFilters(
        widget, event, lastReleaseEvent_.isContinuous(lastPressEvent_), isSpecialEvent);
    if (scriptLine.isEmpty()) {
        scriptLine = filters::qMouseEventFilter(objPath, widget, event);
    }
    else if (duplicateMouseEvent_) {
        static QRegularExpression regex("mouse(Dbl)?Click");
        if (!regex.match(scriptLine).hasMatch()) {
            scriptLine
                += QStringLiteral("// %1").arg(filters::qMouseEventFilter(objPath, widget, event));
        }
    }
    assert(!scriptLine.isEmpty());
    return scriptLine.split('\n');
}

bool UserEventFilter::eventFilter(QObject *reciever, QEvent *event) noexcept
{
    QQuickItem *quickItem = qobject_cast<QQuickItem *>(reciever);
    QWidget *widgetItem = qobject_cast<QWidget *>(reciever);

    // Если оба указателя nullptr, то reciever - не компонент GUI
    if (quickItem == nullptr && widgetItem == nullptr) {
        return QObject::eventFilter(reciever, event);
    }

    // Компонент либо QQuickItem, либо QWidget
    assert(!(quickItem != nullptr && widgetItem != nullptr));

    //! TODO: пока что обрабатываем только QWidget
    if (quickItem != nullptr) {
        return QObject::eventFilter(reciever, event);
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick: {
        const auto path = utils::objectPath(reciever);

        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            if (!lastPressEvent_.registerEvent(std::move(path), event)) {
                break;
            }
            lastReleaseEvent_.clearEvent();

            if (doubleClickTimer_.isActive() && !delayedScriptLines_.isEmpty()) {
                flushScriptLines(delayedScriptLines_);
                delayedScriptLines_.clear();
            }
            doubleClickTimer_.start(QApplication::doubleClickInterval());
            widgetFilter_.setDelayedOrSpecificMouseEventFilter(widgetItem, event);
            break;
        }
        case QEvent::MouseButtonRelease: {
            if (!lastReleaseEvent_.registerEvent(path, event)) {
                break;
            }

            if (doubleClickTimer_.isActive()) {
                delayedScriptLines_ = handleMouseEvent(std::move(path), widgetItem, event);
            }
            else {
                if (doubleClickDetected_) {
                    doubleClickDetected_ = false;
                    if (lastReleaseEvent_.isContinuous(lastPressEvent_)) {
                        flushScriptLines(handleMouseEvent(std::move(path), widgetItem, event));
                    }
                    else if (!delayedScriptLines_.isEmpty()) {
                        flushScriptLines(delayedScriptLines_);
                    }
                }
                else {
                    flushScriptLines(handleMouseEvent(std::move(path), widgetItem, event));
                }
                delayedScriptLines_.clear();
            }
            lastPressEvent_.clearEvent();
            break;
        }
        case QEvent::MouseButtonDblClick: {
            if (!lastPressEvent_.registerEvent(path, event)) {
                break;
            }
            widgetFilter_.setDelayedOrSpecificMouseEventFilter(widgetItem, event);
            doubleClickTimer_.stop();
            doubleClickDetected_ = true;
            delayedScriptLines_ = handleMouseEvent(std::move(path), widgetItem, event);
            lastReleaseEvent_.clearEvent();
            break;
        }
        default:
            Q_UNREACHABLE();
        }
        break;
    }
    case QEvent::Close: {
        flushScriptLines(handleMouseEvent(utils::objectPath(reciever), widgetItem, event, true));
    }
    case QEvent::KeyPress:
        break;
    case QEvent::KeyRelease:
        break;
    default:
        break;
    }

    return QObject::eventFilter(reciever, event);
}
} // namespace QtAda::core
