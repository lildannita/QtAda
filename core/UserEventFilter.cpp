#include "UserEventFilter.hpp"

#include <QApplication>
#include <QQuickItem>
#include <QWidget>
#include <QRegularExpression>

#include "GuiEventFilter.hpp"
#include "utils/CommonFilterUtils.hpp"

//! TODO: remove
#include <iostream>

namespace QtAda::core {
UserEventFilter::UserEventFilter(QObject *parent) noexcept
    : QObject{ parent }
{
    widgetFilter_ = std::make_shared<WidgetEventFilter>(this);

    connect(widgetFilter_.get(), &WidgetEventFilter::newKeyScriptLine, this,
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

MouseEventInfo UserEventFilter::mouseEventInfo(bool isSpecial,
                                               const QString &objPath) const noexcept
{
    MouseEventInfo result;
    result.duplicateMouseEvent = duplicateMouseEvent_;
    result.isSpecialEvent = isSpecial;
    result.isContinuous = lastReleaseEvent_.isContinuous(lastPressEvent_);
    result.objPath = std::move(objPath);
    return result;
}

bool UserEventFilter::eventFilter(QObject *obj, QEvent *event) noexcept
{
    switch (event->type()) {
    // События, для которого нужен currentFilter_:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::KeyPress:
    case QEvent::FocusAboutToChange:
    case QEvent::Close: {
        std::shared_ptr<GuiEventFilterBase> currentFilter_ = nullptr;
        if (qobject_cast<QWidget *>(obj) != nullptr) {
            currentFilter_ = widgetFilter_;
        }
        else if (qobject_cast<QQuickItem *>(obj) != nullptr) {
            //! TODO: пока что обрабатываем только QWidget
            return QObject::eventFilter(obj, event);
        }
        else {
            // Если оба каста не получились, то QObject - не компонент GUI
            return QObject::eventFilter(obj, event);
        }
        assert(currentFilter_ != nullptr);

        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            if (!lastPressEvent_.registerEvent(utils::objectPath(obj), event)) {
                break;
            }
            lastReleaseEvent_.clearEvent();

            if (doubleClickTimer_.isActive() && delayedScriptLine_.has_value()) {
                flushScriptLine(*delayedScriptLine_);
                delayedScriptLine_ = std::nullopt;
            }
            doubleClickTimer_.start(QApplication::doubleClickInterval());
            currentFilter_->setMousePressFilter(obj, event);
            break;
        }
        case QEvent::MouseButtonRelease: {
            const auto path = utils::objectPath(obj);
            if (!lastReleaseEvent_.registerEvent(path, event)) {
                break;
            }

            if (doubleClickTimer_.isActive()) {
                delayedScriptLine_ = currentFilter_->handleMouseEvent(
                    obj, event, mouseEventInfo(false, std::move(path)));
            }
            else {
                if (doubleClickDetected_) {
                    doubleClickDetected_ = false;
                    if (lastReleaseEvent_.isContinuous(lastPressEvent_)) {
                        flushScriptLine(currentFilter_->handleMouseEvent(
                            obj, event, mouseEventInfo(false, std::move(path))));
                    }
                    else if (delayedScriptLine_.has_value()) {
                        flushScriptLine(*delayedScriptLine_);
                    }
                }
                else {
                    flushScriptLine(currentFilter_->handleMouseEvent(
                        obj, event, mouseEventInfo(false, std::move(path))));
                }
                delayedScriptLine_ = std::nullopt;
            }
            lastPressEvent_.clearEvent();
            break;
        }
        case QEvent::MouseButtonDblClick: {
            const auto path = utils::objectPath(obj);
            if (!lastPressEvent_.registerEvent(path, event)) {
                break;
            }
            currentFilter_->setMousePressFilter(obj, event);
            doubleClickTimer_.stop();
            doubleClickDetected_ = true;
            delayedScriptLine_ = currentFilter_->handleMouseEvent(
                obj, event, mouseEventInfo(false, std::move(path)));
            lastReleaseEvent_.clearEvent();
            break;
        }
        case QEvent::KeyPress: {
            if (lastKeyEvent_.registerEvent(utils::objectPath(obj), event)) {
                currentFilter_->handleKeyEvent(obj, event);
            }
            break;
        }
        case QEvent::FocusAboutToChange: {
            //! TODO: надо ли отдельно от KeyPress рассматривать это событие?
            if (lastFocusEvent_.registerEvent(utils::objectPath(obj), event)) {
                currentFilter_->handleKeyEvent(obj, event);
            }
            break;
        }
        case QEvent::Close: {
            flushScriptLine(currentFilter_->handleMouseEvent(obj, event, mouseEventInfo(true)));
            break;
        }
        default:
            Q_UNREACHABLE();
        }
        break;
    }
    case QEvent::KeyRelease: {
        lastKeyEvent_.clearEvent();
        break;
    }
    default:
        break;
    };

    return QObject::eventFilter(obj, event);
}
} // namespace QtAda::core
