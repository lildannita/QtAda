#pragma once

#include "GuiEventFilter.hpp"

namespace QtAda::core {
class QuickEventFilter : public GuiEventFilter<QQuickItem, QuickClass> {
    Q_OBJECT
public:
    QuickEventFilter(QObject *parent = nullptr) noexcept;

    void setMousePressFilter(const QObject *obj, const QEvent *event) noexcept override
    {
        return;
    }
    void handleKeyEvent(const QObject *obj, const QEvent *event) noexcept override
    {
        return;
    }

signals:
    void newScriptKeyLine(const QString &line) const;

private slots:
    void callKeyFilters() noexcept override
    {
        return;
    }

private:
    void processKeyEvent(const QString &text) noexcept override
    {
        return;
    }
    QString callMouseFilters(const QObject *obj, const QEvent *event, bool isContinuous,
                             bool isSpecialEvent) noexcept override;
};
} // namespace QtAda::core
