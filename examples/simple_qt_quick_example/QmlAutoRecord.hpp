#pragma once

#include <QObject>
#include <QPoint>

QT_BEGIN_NAMESPACE
class QQmlApplicationEngine;
class QWindow;
class QTimer;
class QQuickItem;
QT_END_NAMESPACE

class QmlAutoRecord : public QObject {
    Q_OBJECT

public:
    QmlAutoRecord(QQmlApplicationEngine *engine, QObject *parent = nullptr) noexcept;
    void implementActionsForAutoRecord() noexcept;

private:
    QQmlApplicationEngine *engine_ = nullptr;
    QObject *root_ = nullptr;
    QWindow *window_ = nullptr;

    using Action = std::function<void()>;
    QVector<Action> testActions_;
    QTimer *testTimer_ = nullptr;

    void simulateClick(QQuickItem *item, QPoint pos = QPoint()) noexcept;
    void simulateLongPress(QQuickItem *item, QPoint pos = QPoint()) noexcept;
    void simulateKeyClick(QQuickItem *item, bool isDigit = false) noexcept;
};
