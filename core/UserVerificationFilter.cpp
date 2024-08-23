#include "UserVerificationFilter.hpp"

#include <QGuiApplication>
#include <QWidget>
#include <QQuickItem>
#include <QFrame>
#include <regex>

#include "ConfHandler.hpp"
#include "utils/FilterUtils.hpp"
#include "utils/Tools.hpp"

namespace QtAda::core {
static bool fuzzyCompare(qreal left, qreal right)
{
    return (!left || !right) ? qFuzzyIsNull(left - right) : qFuzzyCompare(left, right);
}

static bool checkIfLayout(const QMetaObject *metaObject)
{
    assert(metaObject != nullptr);
    static std::regex layoutRegex("^Q.*Layout$");
    return std::regex_match(metaObject->className(), layoutRegex);
}

static bool hasSameGeometry(const QQuickItem *parent, const QQuickItem *child) noexcept
{
    const auto parentCoordinates = parent->mapToGlobal(QPointF(0, 0));
    const auto childCoordinates = child->mapToGlobal(QPointF(0, 0));
    const auto sameHeight = fuzzyCompare(parent->height(), child->height());
    const auto sameWidth = fuzzyCompare(parent->width(), child->width());
    return parentCoordinates == childCoordinates && sameHeight && sameWidth;
}

static bool hasSameGeometry(const QWidget *parent, const QWidget *child) noexcept
{
    const auto parentCoordinates = parent->mapToGlobal(QPoint(0, 0));
    const auto childCoordinates = child->mapToGlobal(QPoint(0, 0));
    const auto sameHeight = parent->height() == child->height();
    const auto sameWidth = parent->width() == child->width();
    return parentCoordinates == childCoordinates && sameHeight && sameWidth;
}

template <typename GuiComponent>
static std::pair<QObject *, GuiComponent *> findMostSuitableParent(QObject *object,
                                                                   GuiComponent *component) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);

    assert(component != nullptr);

    auto *foundParent = object;
    auto *foundParentComponent = component;

    auto *current = component->parent();
    while (current != nullptr) {
        auto *currentComponent = qobject_cast<GuiComponent *>(current);
        if (currentComponent == nullptr) {
            break;
        }

        if (!checkIfLayout(currentComponent->metaObject())
            && hasSameGeometry(currentComponent, component)) {
            foundParent = current;
            foundParentComponent = currentComponent;
        }
        current = current->parent();
    }

    assert(foundParent != nullptr);
    assert(foundParentComponent != nullptr);
    return std::make_pair(foundParent, foundParentComponent);
}

static QList<QVariantMap> serilizeMetaPropertyData(const QObject *object) noexcept
{
    assert(object != nullptr);
    QList<QVariantMap> metaPropertyData;

    const auto *metaObject = object->metaObject();
    assert(metaObject != nullptr);
    const auto propertyCount = metaObject->propertyCount();
    for (int i = 0; i < propertyCount; i++) {
        const auto metaProperty = metaObject->property(i);
        assert(metaProperty.isValid());

        QVariantMap metaRow;
        metaRow["property"] = metaProperty.name();
        metaRow["value"] = tools::metaPropertyValueToString(object, metaProperty);
        metaPropertyData.append(metaRow);
    }
    return metaPropertyData;
}

std::optional<QVariantMap>
UserVerificationFilter::updateFramedRootObjectModel(const QObject *object,
                                                    QStandardItem *parentViewItem) noexcept
{
    assert(object != nullptr);
    QVariantMap rowMap;
    if (object == qobject_cast<const QObject *>(lastFrame_)
        || object == qobject_cast<const QObject *>(lastPaintedItem_)) {
        return std::nullopt;
    }

    const auto objectName = object->objectName();
    rowMap["object"] = QStringLiteral("%1 (%2)")
                           .arg(objectName.isEmpty() ? tools::pointerToString(object) : objectName)
                           .arg(object->metaObject()->className());
    //! TODO: заменить в этом и подобных местах "path" на "id"
    rowMap["path"] = ConfHandler::getObjectId(object);

    auto *viewItem = new QStandardItem();
    viewItem->setData(QVariant::fromValue(const_cast<QObject *>(object)), Qt::UserRole);
    if (parentViewItem == nullptr) {
        framedRootObjectModel_.appendRow(viewItem);
    }
    else {
        parentViewItem->appendRow(viewItem);
    }

    QVariantList childrenData;
    for (auto *child : object->children()) {
        const auto childMap = updateFramedRootObjectModel(child, viewItem);
        if (childMap.has_value()) {
            childrenData.append(std::move(*childMap));
        }
    }

    if (!childrenData.isEmpty()) {
        rowMap["children"] = childrenData;
    }

    return rowMap;
}

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

    framedRootObjectModel_.clear();
}

bool UserVerificationFilter::handleWidgetVerification(QWidget *widget, bool isExtTrigger) noexcept
{
    assert(widget != nullptr);
    if (lastFrame_ != nullptr && lastFrame_->parentWidget() == widget) {
        return false;
    }

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

    return true;
}

bool UserVerificationFilter::handleItemVerification(QQuickItem *item, bool isExtTrigger) noexcept
{
    assert(item != nullptr);
    if (lastPaintedItem_ != nullptr && lastPaintedItem_->parentItem() == item) {
        return false;
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

    return true;
}

void UserVerificationFilter::callHandlers(QObject *obj, bool isExtTrigger) noexcept
{
    //! TODO: Сейчас рисуем рамку только около компонентов, которые могут быть
    //! преобразованы в QWidget или в QQuickItem, так как для других компонентов
    //! не можем "легко" получить геометрию. QObject, которые не можем преобразовать
    //! в графический компонент, мы все равно включаем в дерево потомков выбранного
    //! графического компонента, и все равно отображаем его мета-информацию, но
    //! так как это не "явный" графический компонент, то и рамку для него не рисуем.
    //!
    //! Проблема в том, что QObject все равно может быть отображен в GUI, как, например,
    //! делегаты для QtWidgets. В этом случае его родитель (являющийся графическим
    //! компонентом) занимается отображением QObject внутри себя. И по идее в таких
    //! случаях можно "обновить" рамку, используя координаты QObject внутри графического
    //! компонента (если они есть).

    bool isFrameUpdated = false;
    if (auto *widget = qobject_cast<QWidget *>(obj)) {
        if (!isExtTrigger) {
            std::tie(obj, widget) = findMostSuitableParent(obj, widget);
        }
        isFrameUpdated = handleWidgetVerification(widget, isExtTrigger);
    }
    else if (auto *item = qobject_cast<QQuickItem *>(obj)) {
        if (!isExtTrigger) {
            std::tie(obj, item) = findMostSuitableParent(obj, item);
        }
        isFrameUpdated = handleItemVerification(item, isExtTrigger);
    }

    const auto metaPropertyData = serilizeMetaPropertyData(obj);
    if (isFrameUpdated && !isExtTrigger) {
        framedRootObjectModel_.clear();
        const auto serializedRootModel = updateFramedRootObjectModel(obj, nullptr);
        assert(serializedRootModel.has_value());
        emit newFramedRootObjectData(std::move(*serializedRootModel), std::move(metaPropertyData));
    }
    else {
        emit newMetaPropertyData(std::move(metaPropertyData));
    }
}

void UserVerificationFilter::changeFramedObject(const QList<int> &rowPath) noexcept
{
    if (framedRootObjectModel_.rowCount() == 0) {
        return;
    }

    auto path = rowPath;
    auto newFramedObjectIndex = framedRootObjectModel_.index(path.takeFirst(), 0);
    for (const auto row : path) {
        assert(newFramedObjectIndex.isValid());
        newFramedObjectIndex = framedRootObjectModel_.index(row, 0, newFramedObjectIndex);
    }
    assert(newFramedObjectIndex.isValid());

    auto *newFramedObject = newFramedObjectIndex.data(Qt::UserRole).value<QObject *>();
    callHandlers(newFramedObject, true);
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
        if (lastPressEvent_.registerEvent(ConfHandler::getObjectPath(obj), event)) {
            callHandlers(obj, false);
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
