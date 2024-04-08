#include "UserEventFilter.hpp"

#include <QApplication>
#include <QQuickItem>
#include <QWidget>
#include <QRegularExpression>

#include "utils/FilterUtils.hpp"

//! TODO: remove
#include <iostream>

namespace QtAda::core {
UserEventFilter::UserEventFilter(QObject *parent) noexcept
    : QObject{ parent }
{
    connect(&widgetFilter_, &WidgetEventFilter::newScriptKeyLine, this,
            &UserEventFilter::newScriptLine);

    doubleClickTimer_.setSingleShot(true);
    connect(&doubleClickTimer_, &QTimer::timeout, this, [this]() {
        if (delayedScriptLine_.has_value()) {
            flushScriptLine(*delayedScriptLine_);
            delayedScriptLine_ = std::nullopt;
        }
    });

    //! TODO: убрать
    connect(this, &UserEventFilter::newScriptLine, this,
            [](const QString &line) { std::cout << line.toStdString() << std::endl; });
}

void UserEventFilter::flushScriptLine(const QString &line) const noexcept
{
    assert(!line.isEmpty());
    emit newScriptLine(line);
}

QString UserEventFilter::handleMouseEvent(const QString &objPath, const QWidget *widget,
                                          const QEvent *event, bool isSpecialEvent) noexcept
{
    auto scriptLine = widgetFilter_.callWidgetMouseFilters(
        widget, event, lastReleaseEvent_.isContinuous(lastPressEvent_), isSpecialEvent);
    if (scriptLine.isEmpty()) {
        scriptLine = filters::qMouseEventFilter(objPath, widget, event);
    }
    else if (duplicateMouseEvent_) {
        static QRegularExpression s_regex("mouse(Dbl)?Click");
        if (!s_regex.match(scriptLine).hasMatch()) {
            scriptLine
                += QStringLiteral("// %1").arg(filters::qMouseEventFilter(objPath, widget, event));
        }
    }
    assert(!scriptLine.isEmpty());
    return scriptLine;
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
        if (event->type() == QEvent::MouseButtonPress)
            while (quickItem != nullptr) {
                const auto *metaObject = quickItem->metaObject();
                while (metaObject != nullptr) {
                    std::cout << metaObject->className() << std::endl;
                    metaObject = metaObject->superClass();
                }
                quickItem = quickItem->parentItem();
            }

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

            if (doubleClickTimer_.isActive() && delayedScriptLine_.has_value()) {
                flushScriptLine(*delayedScriptLine_);
                delayedScriptLine_ = std::nullopt;
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
                delayedScriptLine_ = handleMouseEvent(std::move(path), widgetItem, event);
            }
            else {
                if (doubleClickDetected_) {
                    doubleClickDetected_ = false;
                    if (lastReleaseEvent_.isContinuous(lastPressEvent_)) {
                        flushScriptLine(handleMouseEvent(std::move(path), widgetItem, event));
                    }
                    else if (delayedScriptLine_.has_value()) {
                        flushScriptLine(*delayedScriptLine_);
                    }
                }
                else {
                    flushScriptLine(handleMouseEvent(std::move(path), widgetItem, event));
                }
                delayedScriptLine_ = std::nullopt;
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
            delayedScriptLine_ = handleMouseEvent(std::move(path), widgetItem, event);
            lastReleaseEvent_.clearEvent();
            break;
        }
        default:
            Q_UNREACHABLE();
        }
        break;
    }
    case QEvent::Close: {
        flushScriptLine(handleMouseEvent(utils::objectPath(reciever), widgetItem, event, true));
        break;
    }
    case QEvent::KeyPress: {
        if (lastKeyEvent_.registerEvent(utils::objectPath(reciever), event)) {
            widgetFilter_.updateKeyWatchDog(widgetItem, event);
        }
        break;
    }
    case QEvent::KeyRelease: {
        lastKeyEvent_.clearEvent();
        break;
    }
    case QEvent::FocusAboutToChange: {
        //! TODO: надо ли отдельно от KeyPress рассматривать это событие?
        if (lastFocusEvent_.registerEvent(utils::objectPath(reciever), event)) {
            widgetFilter_.updateKeyWatchDog(widgetItem, event);
        }
        break;
    }
    default:
        break;
    }

    return QObject::eventFilter(reciever, event);
}
} // namespace QtAda::core
