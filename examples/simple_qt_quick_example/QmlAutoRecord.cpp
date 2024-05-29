#include "QmlAutoRecord.hpp"

#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QTimer>
#include <QtTest>
#include <QWindow>
#include <QQuickWindow>

QmlAutoRecord::QmlAutoRecord(QQmlApplicationEngine *engine, QObject *parent) noexcept
    : QObject{ parent }
    , engine_{ engine }
{
    assert(engine_ != nullptr);
    root_ = engine_->rootObjects().first();
    assert(root_ != nullptr);
    window_ = qobject_cast<QQuickWindow *>(root_);
}

static QMouseEvent *simpleMouseEvent(const QEvent::Type type, const QPoint &pos,
                                     const Qt::MouseButton button = Qt::LeftButton) noexcept
{
    return new QMouseEvent(type, pos, button, button, Qt::NoModifier);
}

static QPoint getSpecificClickPoint(QQuickItem *item, double dx, double dy) noexcept
{
    return item->mapToScene(QPoint(0, 0)).toPoint() + QPoint(dx, dy);
}

static QPoint getClickPoint(QQuickItem *item, double dx = 0.5, double dy = 0.5) noexcept
{
    return item->mapToScene(QPoint(0, 0)).toPoint()
           + QPoint(item->width() * dx, item->height() * dy);
}

void QmlAutoRecord::simulateClick(QQuickItem *item, QPoint pos) noexcept
{
    if (pos.isNull()) {
        pos = getClickPoint(item);
    }
    QTest::mouseClick(window_, Qt::LeftButton, Qt::NoModifier, pos);
}

void QmlAutoRecord::simulateLongPress(QQuickItem *item, QPoint pos) noexcept
{
    if (pos.isNull()) {
        pos = getClickPoint(item);
    }
    QTest::mousePress(window_, Qt::LeftButton, Qt::NoModifier, pos);
    QTest::qWait(700);
    QTest::mouseRelease(window_, Qt::LeftButton, Qt::NoModifier, pos);
}

void QmlAutoRecord::simulateKeyClick(QQuickItem *item, bool isDigit) noexcept
{
    simulateClick(item);
    QTest::qWait(100);
    QTest::keyClick(window_, isDigit ? '7' : 'D');
    // Согласно логике анализа действий, нужно перевести фокус или выполнить любое нажатие, чтобы
    // действие точно записалось
    item->setFocus(false);
}

void QmlAutoRecord::implementActionsForAutoRecord() noexcept
{
    testActions_ = {
        /********** ПЕРВАЯ СТРАНИЦА **********/
        //! TODO: Это действие открывает диалог, но пока непонятно, как работать с
        //! диалогами в QML, поскольку получить их их engine_ не удается
        //! [this] {
        //!     auto *item = root_->findChild<QQuickItem*>("customButton");
        //!     assert(item != nullptr);
        //!     auto *clickableItem = item->findChild<QQuickItem*>("customMouseArea");
        //!     assert(clickableItem != nullptr);
        //!     simulateClick(clickableItem);
        //! },
        // Нажатие на кнопку "Checkable Button" (переводим в состояние true)
        [this] {
            auto *item = root_->findChild<QQuickItem *>("checkableButton");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на кнопку "Checkable Button" (переводим в состояние false)
        [this] {
            auto *item = root_->findChild<QQuickItem *>("checkableButton");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на первый CheckBox (переводим в состояние true)
        [this] {
            auto *item = root_->findChild<QQuickItem *>("firstCheckBox");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на второй CheckBox (переводим в состояние true)
        [this] {
            auto *item = root_->findChild<QQuickItem *>("secondCheckBox");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на недоступный CheckBox
        [this] {
            auto *item = root_->findChild<QQuickItem *>("disabledCheckBox");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на второй CheckBox (переводим в состояние false)
        [this] {
            auto *item = root_->findChild<QQuickItem *>("secondCheckBox");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на первый RadioButton
        [this] {
            auto *item = root_->findChild<QQuickItem *>("firstRadioButton");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на второй RadioButton
        [this] {
            auto *item = root_->findChild<QQuickItem *>("secondRadioButton");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на недоступный RadioButton
        [this] {
            auto *item = root_->findChild<QQuickItem *>("disabledRadioButton");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на Switch (переводим в состояние true)
        [this] {
            auto *item = root_->findChild<QQuickItem *>("simpleSwitch");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на недоступный Switch
        [this] {
            auto *item = root_->findChild<QQuickItem *>("disabledSwitch");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на Switch (переводим в состояние false)
        [this] {
            auto *item = root_->findChild<QQuickItem *>("simpleSwitch");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Зажатие DelayButton
        [this] {
            auto *item = root_->findChild<QQuickItem *>("delayButton");
            assert(item != nullptr);
            simulateLongPress(item);
        },
        // Изменение значения в Slider
        [this] {
            auto *item = root_->findChild<QQuickItem *>("simpleSlider");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Изменение левого значения в RangeSlider
        [this] {
            auto *item = root_->findChild<QQuickItem *>("rangeSlider");
            assert(item != nullptr);
            simulateClick(item, getClickPoint(item, 0.2, 0.5));
        },
        // Изменение правого значения в RangeSlider
        [this] {
            auto *item = root_->findChild<QQuickItem *>("rangeSlider");
            assert(item != nullptr);
            simulateClick(item, getClickPoint(item, 0.8, 0.5));
        },
        // Изменение значения в Dial
        [this] {
            auto *item = root_->findChild<QQuickItem *>("simpleDial");
            assert(item != nullptr);
            simulateClick(item);
        },
        //! TODO: Нужно придумать, как тестировать выбор элемента в ComboBox для QML
        // Изменяем текст в "editable" ComboBox
        [this] {
            auto *item = root_->findChild<QQuickItem *>("editableComboBox");
            assert(item != nullptr);
            simulateKeyClick(item);
        },
        // Увеличиваем на один шаг значение в SpinBox
        [this] {
            auto *item = root_->findChild<QQuickItem *>("simpleSpinBox");
            assert(item != nullptr);
            simulateClick(item, getSpecificClickPoint(item, item->width() - 5, item->height() / 2));
        },
        // Добавляем вручную цифру в SpinBox
        [this] {
            auto *item = root_->findChild<QQuickItem *>("simpleSpinBox");
            assert(item != nullptr);
            simulateKeyClick(item, true);
        },
        // Уменьшаем на несколько шагов значение в SpinBox
        [this] {
            auto *item = root_->findChild<QQuickItem *>("simpleSpinBox");
            assert(item != nullptr);
            simulateLongPress(item, getSpecificClickPoint(item, 5, item->height() / 2));
        },
        // Увеличиваем на несколько шагов значение в "Double" SpinBox
        [this] {
            auto *item = root_->findChild<QQuickItem *>("doubleSpinBox");
            assert(item != nullptr);
            simulateLongPress(item,
                              getSpecificClickPoint(item, item->width() - 5, item->height() / 2));
        },
        // Уменьшаем на один шаг значение в "Double" SpinBox
        [this] {
            auto *item = root_->findChild<QQuickItem *>("doubleSpinBox");
            assert(item != nullptr);
            simulateClick(item, getSpecificClickPoint(item, 5, item->height() / 2));
        },
        // Изменяем положение ScrollBar
        [this] {
            auto *item = root_->findChild<QQuickItem *>("simpleScrollBar");
            assert(item != nullptr);
            simulateClick(item, getClickPoint(item, 0.75, 0.5));
        },

        /********** ВТОРАЯ СТРАНИЦА **********/
        // Переключаем на вторую страницу
        [this] {
            auto *item = root_->findChild<QQuickItem *>("textItemsTabButton");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Вставка текста в TextInput
        [this] {
            auto *item = root_->findChild<QQuickItem *>("simpleTextInput");
            assert(item != nullptr);
            simulateKeyClick(item);
        },
        // Вставка текста в кастомный TextInput
        [this] {
            auto *item = root_->findChild<QQuickItem *>("customTextInput");
            assert(item != nullptr);
            simulateKeyClick(item, true);
        },
        // Вставка текста в TextField
        [this] {
            auto *item = root_->findChild<QQuickItem *>("simpleTextField");
            assert(item != nullptr);
            simulateKeyClick(item);
        },
        // Вставка текста в TextEdit
        [this] {
            auto *item = root_->findChild<QQuickItem *>("simpleTextEdit");
            assert(item != nullptr);
            simulateKeyClick(item, true);
        },
        // Вставка текста в TextArea
        [this] {
            auto *item = root_->findChild<QQuickItem *>("simpleTextArea");
            assert(item != nullptr);
            simulateKeyClick(item);
        },

        /********** ТРЕТЬЯ СТРАНИЦА **********/
        // Переключаем на третью страницу
        [this] {
            auto *item = root_->findChild<QQuickItem *>("viewItemsTabButton");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на вкладку GridView
        [this] {
            auto *item = root_->findChild<QQuickItem *>("tabGridView");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на вкладку ListView
        [this] {
            auto *item = root_->findChild<QQuickItem *>("tabListView");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на вкладку PathView
        [this] {
            auto *item = root_->findChild<QQuickItem *>("tabPathView");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на вкладку SwipeView
        [this] {
            auto *item = root_->findChild<QQuickItem *>("tabSwipeView");
            assert(item != nullptr);
            simulateClick(item);
        },
        // Нажатие на вкладку TableView
        [this] {
            auto *item = root_->findChild<QQuickItem *>("tabTableView");
            assert(item != nullptr);
            simulateClick(item);
        },
        //! TODO: Позже нужно придумать как быстро и "полезно" (для автотестов)
        //! взаимодействовать с элементами модели.
    };

    testTimer_ = new QTimer(this);
    testTimer_->setInterval(500);
    connect(testTimer_, &QTimer::timeout, this, [this] {
        if (testActions_.empty()) {
            testTimer_->stop();
            QGuiApplication::exit(0);
        }
        else {
            auto action = testActions_.takeFirst();
            action();
        }
    });

    // Откладываем запуск "сценария"
    QTimer::singleShot(1500, [this] { testTimer_->start(); });
}
