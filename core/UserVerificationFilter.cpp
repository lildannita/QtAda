#include "UserVerificationFilter.hpp"

#include <QGuiApplication>
#include <QWidget>
#include <QQuickItem>
#include <QFrame>
#include <regex>

#include "utils/FilterUtils.hpp"
#include "utils/Tools.hpp"

namespace QtAda::core {
static bool checkIfLayout(const QMetaObject *metaObject)
{
    assert(metaObject != nullptr);
    static std::regex layoutRegex("^Q.*Layout$");
    return std::regex_match(metaObject->className(), layoutRegex);
}

template <typename GuiComponent>
static GuiComponent *findMostSuitableParent(GuiComponent *component) noexcept
{
    CHECK_GUI_CLASS(GuiComponent);

    if (component == nullptr) {
        return nullptr;
    }

    auto *foundParentComponent = component;
    auto *parent = component->parent();
    while (parent) {
        auto *parentComponent = qobject_cast<GuiComponent *>(parent);
        parent = parent->parent();
        if (parentComponent == nullptr) {
            break;
        }

        if (!checkIfLayout(parentComponent->metaObject()) && component->x() == parentComponent->x()
            && component->y() == parentComponent->y()
            && component->height() == parentComponent->height()
            && component->width() == parentComponent->width()) {
            foundParentComponent = parentComponent;
        }
    }
    return foundParentComponent;
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
    rowMap["path"] = utils::objectPath(object);

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
    if (!isExtTrigger) {
        widget = findMostSuitableParent(widget);
    }
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
    if (!isExtTrigger) {
        item = findMostSuitableParent(item);
    }
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
        isFrameUpdated = handleWidgetVerification(widget, isExtTrigger);
    }
    else if (auto *item = qobject_cast<QQuickItem *>(obj)) {
        isFrameUpdated = handleItemVerification(item, isExtTrigger);
    }
    else {
        if (!isExtTrigger) {
            return;
        }
        const auto metaPropertyData = serilizeMetaPropertyData(obj);
        emit newMetaPropertyData(std::move(metaPropertyData));
    }

    if (!isFrameUpdated) {
        return;
    }

    const auto metaPropertyData = serilizeMetaPropertyData(obj);
    if (!isExtTrigger) {
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
    QModelIndex newFramedObjectIndex = framedRootObjectModel_.index(path.takeFirst(), 0);
    for (const auto row : rowPath) {
        newFramedObjectIndex = framedRootObjectModel_.index(row, 0, newFramedObjectIndex);
        if (!newFramedObjectIndex.isValid()) {
            return;
        }
    }

    if (!newFramedObjectIndex.isValid()) {
        return;
    }

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
        if (lastPressEvent_.registerEvent(utils::objectPath(obj), event)) {
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
