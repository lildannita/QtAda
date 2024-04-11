#pragma once

#include <QtGlobal>
#include <functional>
#include <optional>
#include <variant>

#include <QObject>
#include <QEvent>
#include <QModelIndex>
#include <QTimer>

#include "GuiEventFilter.hpp"
#include "ProcessedObjects.hpp"

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QString;
class QWidget;
QT_END_NAMESPACE

namespace QtAda::core {
class WidgetEventFilter final : public GuiEventFilter<QWidget, WidgetClass> {
    Q_OBJECT
public:
    WidgetEventFilter(QObject *parent = nullptr) noexcept;

    void setMousePressFilter(const QObject *obj, const QEvent *event) noexcept override;
    void handleKeyEvent(const QObject *obj, const QEvent *event) noexcept override;

signals:
    void newScriptKeyLine(const QString &line) const;

private slots:
    void callWidgetKeyFilters() noexcept override;

private:
    void processKeyEvent(const QString &text) noexcept override;
    QString callMouseFilters(const QObject *obj, const QEvent *event, bool isContinuous,
                             bool isSpecialEvent) noexcept override;
};
} // namespace QtAda::core
