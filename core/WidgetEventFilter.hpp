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
    QString handleCloseEvent(const QObject *obj, const QEvent *event) noexcept override;

signals:
    void newScriptKeyLine(const QString &line) const;

private slots:
    void callKeyFilters() noexcept override;

private:
    std::vector<MouseFilterFunction> specificMouseFilters_;

    void processKeyEvent(const QString &text) noexcept override;
    std::pair<QString, bool> callMouseFilters(const QObject *obj, const QEvent *event,
                                              bool isContinuous) noexcept override;
};
} // namespace QtAda::core
