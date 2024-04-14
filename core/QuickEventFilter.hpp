#pragma once

#include "GuiEventFilter.hpp"

namespace QtAda::core {
class QuickEventFilter : public GuiEventFilter<QQuickItem, QuickClass> {
    Q_OBJECT
public:
    QuickEventFilter(QObject *parent = nullptr) noexcept;

    void setMousePressFilter(const QObject *obj, const QEvent *event) noexcept override;
    void handleKeyEvent(const QObject *obj, const QEvent *event) noexcept override
    {
        return;
    }

signals:
    void newScriptKeyLine(const QString &line) const;

private slots:
    //! TODO: на текущий момент для delayed QtQuick-компонентов приходится использовать
    //! "стандартную" систему `сигнал-слот`, из-за чего приходится явно прописывать слоты.
    void classicCallSlot() noexcept
    {
        delayedData_.processSignal();
    }

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
