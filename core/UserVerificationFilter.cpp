#include "UserVerificationFilter.hpp"

#include <QGuiApplication>
#include <QWidget>
#include <QQuickItem>
#include <QFrame>

#include "utils/FilterUtils.hpp"

namespace QtAda::core {
void UserVerificationFilter::cleanupFrames() noexcept
{
    if (lastFrame_ != nullptr) {
        lastFrame_->hide();
        lastFrame_->deleteLater();
        lastFrame_ = nullptr;
    }

    if (lastPaintedItem_ != nullptr) {
        lastPaintedItem_->deleteLater();
        lastPaintedItem_ = nullptr;
    }
}

void UserVerificationFilter::handleWidgetVerification(QWidget *widget) noexcept
{
    if (lastFrame_ == nullptr) {
        lastFrame_ = new QFrame(widget);
        // Не можем настраивать в конструкторе, так как если тестируемое приложение
        // не построено на QApplication, то и QFrame создать не получится.
        lastFrame_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        //! TODO: странно работает при первом нажатии:
        //! lastFrame_->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
        lastFrame_->setFrameShape(QFrame::Box);
        lastFrame_->setFrameShadow(QFrame::Raised);
        lastFrame_->setStyleSheet("QFrame { "
                                  "  border: 3px solid rgb(217, 4, 41); "
                                  "  background-color: rgba(239, 35, 60, 127); "
                                  "}");
    }
    else {
        lastFrame_->hide();
    }
    lastFrame_->setParent(widget);
    lastFrame_->setGeometry(widget->rect());
    lastFrame_->show();
}

void UserVerificationFilter::handleItemVerification(QQuickItem *item) noexcept
{
    if (lastPaintedItem_ == item) {
        return;
    }

    if (lastPaintedItem_ == nullptr) {
        lastPaintedItem_ = new QuickFrame(item);
    }
    else {
        lastPaintedItem_->setParentItem(item);
    }
    lastPaintedItem_->setWidth(item->width());
    lastPaintedItem_->setHeight(item->height());
    lastPaintedItem_->update();
}

bool UserVerificationFilter::eventFilter(QObject *obj, QEvent *event) noexcept
{
    // Так как в QtQuick отличная от QtWidgets логика сигналов, то первый "источник"
    // нажатия - QQuickWindow, и если игнорировать нажатия в нем, то дальше сигнал не пойдет.
    if (utils::metaObjectWatcher(obj->metaObject(), QLatin1String("QQuickWindow"))) {
        return QObject::eventFilter(obj, event);
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        if (!lastPressEvent_.registerEvent(utils::objectPath(obj), event)) {
            return true;
        }

        if (auto *widget = qobject_cast<QWidget *>(obj)) {
            handleWidgetVerification(widget);
        }
        else if (auto *item = qobject_cast<QQuickItem *>(obj)) {
            handleItemVerification(item);
        }
        return true;
    }
    //! TODO: пока нет четкого понимания как правильно блокировать все пользовательские
    //! события таким образом, чтобы могла остаться отрисовка QFrame. Также существует
    //! проблема, что несмотря на все заблокированные события мыши, все равно при
    //! нажатии на кнопки (например, QPushButton) они перерисовываются так, будто на
    //! них перенаправлен фокус, хотя это не так...
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    case QEvent::FocusAboutToChange:
    case QEvent::Enter:
    case QEvent::Leave:
    case QEvent::Wheel:
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::DragLeave:
    case QEvent::DragResponse:
    case QEvent::Drop:
    case QEvent::OkRequest:
    case QEvent::HelpRequest:
    case QEvent::Shortcut:
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
    case QEvent::ApplicationStateChange:
    case QEvent::ApplicationActivate:
    case QEvent::WindowActivate:
    case QEvent::PaletteChange:
    case QEvent::CursorChange:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        return true;
    default:
        return QObject::eventFilter(obj, event);
    }
}
} // namespace QtAda::core
