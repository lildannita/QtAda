#include "ScreenshotManager.hpp"

#include <QGuiApplication>
#include <QScreen>
#include <QDir>
#include <QDateTime>
#include <QElapsedTimer>
#include <QWidget>
#include <QQuickItem>
#include <QQuickWindow>

namespace QtAda::core {
static constexpr int DEFAULT_SCREENSHOT_TIMEOUT_MS = 5000;
static constexpr int MAX_SCREENSHOTS = 10;

static QString generateDirPath(const QString &dirPath, const QString &scriptName) noexcept
{
    const QString originalDirPath
        = dirPath + QDir::separator()
          + QStringLiteral("qtada_screenshot_backtrace_%1_%2")
                .arg(scriptName, QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss-zzz"));
    auto newDirPath = originalDirPath;

    uint count = 0;
    QDir newDirInfo(newDirPath);
    while (newDirInfo.exists()) {
        newDirPath = QStringLiteral("%1_%2").arg(originalDirPath).arg(++count);
        newDirInfo = QDir(newDirPath);
    }

    return newDirPath;
}

static QObject *getParent(const QObject *object) noexcept
{
    auto classicParent = object->parent();
    if (classicParent != nullptr) {
        return classicParent;
    }

    if (auto quickItem = qobject_cast<const QQuickItem *>(object)) {
        return qobject_cast<QObject *>(quickItem->parentItem());
    }
    return nullptr;
}

static QObject *getParentWindow(const QObject *object) noexcept
{
    auto parent = getParent(object);
    while (parent != nullptr) {
        if (const auto *widget = qobject_cast<const QWidget *>(parent)) {
            if (widget->isWindow()) {
                return parent;
            }
        }

        if (const auto *item = qobject_cast<const QQuickWindow *>(parent)) {
            return parent;
        }
        parent = getParent(object);
    }
    return nullptr;
}

ScreenshotManager::ScreenshotManager(const QString &dirPath, const QString &scriptName,
                                     QObject *parent) noexcept
    : QObject{ parent }
    , dirPath_{ generateDirPath(dirPath, scriptName) }
{
    QDir dir;
    bool dirCreated = dir.mkpath(dirPath_);
    assert(dirCreated == true);
}

void ScreenshotManager::newScreenshot(const QObject *object, const QString &funcName) noexcept
{
    bool success = false;
    const auto screenPath
        = QDir(dirPath_).filePath(QStringLiteral("%1_%2.png").arg(count_).arg(funcName));
    if (const auto *widget = qobject_cast<const QWidget *>(object)) {
        success = screenshot(widget, screenPath);
    }
    else if (const auto *quickItem = qobject_cast<const QQuickItem *>(object)) {
        success = screenshot(quickItem, screenPath);
    }
    if (!success) {
        success = screenshotByObject(object, screenPath);
    }

    if (success) {
        screenshots_.push_back(screenPath);
        handleScreenshotsDir();
        count_++;
    }
    //! TODO: как-нибудь обработать ситуацию, когда не получился скриншот
}

void ScreenshotManager::handleScreenshotsDir() noexcept
{
    if (screenshots_.size() <= MAX_SCREENSHOTS) {
        return;
    }
    const auto oldestScreenshot = screenshots_.front();
    screenshots_.pop_front();
    if (QFile::exists(oldestScreenshot)) {
        bool ok = QFile::remove(oldestScreenshot);
        assert(ok == true);
    }
}

void ScreenshotManager::clearScreenshots() noexcept
{
    QDir dir(dirPath_);
    assert(dir.exists());
    dir.removeRecursively();
    screenshots_.clear();
}

template <typename T>
bool ScreenshotManager::screenshot(const T *item, const QString &path) const noexcept
{
    QElapsedTimer timer;
    timer.start();
    auto *window = item->window();
    while (window == nullptr && !timer.hasExpired(DEFAULT_SCREENSHOT_TIMEOUT_MS)) {
        window = item->window();
    }
    if (!window) {
        return false;
    }
    return do_screenshot(window, path);
}

bool ScreenshotManager::do_screenshot(QWidget *window, const QString &path) const noexcept
{
    if (window == nullptr) {
        return false;
    }
    bool success = false;
    bool ok = QMetaObject::invokeMethod(
        window,
        [window, path, &success]() {
            const auto screenshot = window->grab();
            success = !screenshot.isNull() && screenshot.save(path);
        },
        Qt::BlockingQueuedConnection);
    assert(ok == true);
    return success;
}

bool ScreenshotManager::do_screenshot(QQuickWindow *window, const QString &path) const noexcept
{
    if (window == nullptr) {
        return false;
    }
    bool success = false;
    bool ok = QMetaObject::invokeMethod(
        window,
        [window, path, &success]() {
            const auto screenshot = window->grabWindow();
            success = !screenshot.isNull() && screenshot.save(path);
        },
        Qt::BlockingQueuedConnection);
    assert(ok == true);
    return success;
}

bool ScreenshotManager::screenshotByObject(const QObject *object,
                                           const QString &path) const noexcept
{
    auto *window = getParentWindow(object);
    if (window == nullptr) {
        return false;
    }
    if (auto *widgetWindow = qobject_cast<QWidget *>(window)) {
        return do_screenshot(widgetWindow, path);
    }
    else if (auto *quickWindow = qobject_cast<QQuickWindow *>(window)) {
        return do_screenshot(quickWindow, path);
    }
    return false;
}

void ScreenshotManager::screenshotAllWindowsByError(int errorLineNumber) const noexcept
{
    // В этом случае мы скриним все открытые окна + забиваем на ограничения
    // по количеству скринов
    uint windowNumber = 0;
    const auto windows = QGuiApplication::allWindows();
    for (auto *window : windows) {
        windowNumber++;

        QElapsedTimer timer;
        timer.start();

        auto screenshot = window->screen()->grabWindow(window->winId());
        while (screenshot.isNull() && !timer.hasExpired(DEFAULT_SCREENSHOT_TIMEOUT_MS)) {
            screenshot = window->screen()->grabWindow(window->winId());
        }

        if (screenshot.isNull()) {
            continue;
        }

        auto windowTitle = window->title();
        if (windowTitle.isEmpty()) {
            windowTitle = QStringLiteral("window_%1").arg(windowNumber);
        }
        const auto screenPath = QDir(dirPath_).filePath(
            QStringLiteral("ERROR_LINE_%1_%2.png").arg(errorLineNumber).arg(windowTitle));
        assert(QFileInfo::exists(screenPath) == false);
        screenshot.save(screenPath);
    }
}
} // namespace QtAda::core
