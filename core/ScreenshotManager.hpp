#pragma once

#include <QObject>
#include <deque>

QT_BEGIN_NAMESPACE
class QWidget;
class QQuickItem;
class QQuickWindow;
QT_END_NAMESPACE

//! TODO: Пока что этот класс сохраняет скрины сразу в память устройства,
//! а не хранит в оперативной памяти. Это не совсем хорошо + медленно ->
//! -> желательно хранить в оперативной памяти и в случае падения выгружать
//! на диск. Но пока так не можем сделать, потому что:
//! 1.  Нужно отлавливать внутренние ошибки тестируемого приложения. Сейчас
//!     инъекция падает вместе с приложением -> не успеваем выгрузить
//!     скриншоты.
//! 2.  Обратная ситуация: QtAda пока не стабильна, часто ловим Process crashed
//!     -> не успеваем выгрузить скриншоты.

namespace QtAda::core {
class ScreenshotManager : public QObject {
    Q_OBJECT
public:
    ScreenshotManager(const QString &dirPath, const QString &scriptName,
                      QObject *parent = nullptr) noexcept;
    void newScreenshot(const QObject *object, const QString &funcName) noexcept;
    void screenshotAllWindowsByError(int errorLineNumber) const noexcept;
    void clearScreenshots() noexcept;

private:
    const QString dirPath_;
    uint64_t count_ = 0;
    std::deque<QString> screenshots_;

    template <typename T> bool screenshot(const T *item, const QString &path) const noexcept;
    bool do_screenshot(QWidget *window, const QString &path) const noexcept;
    bool do_screenshot(QQuickWindow *window, const QString &path) const noexcept;
    bool screenshotByObject(const QObject *object, const QString &path) const noexcept;

    void handleScreenshotsDir() noexcept;
};
} // namespace QtAda::core
