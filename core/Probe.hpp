#pragma once

#include <QObject>
// #include <QSet>
#include <set>

namespace QtAda::core {
class Probe : public QObject {
    Q_OBJECT

public:
    explicit Probe(QObject *parent = nullptr) noexcept;
    ~Probe() noexcept;

    static bool initialized() noexcept;
    static void initProbe() noexcept;
    static void addObject(QObject *obj) noexcept;
    static void removeObject(QObject *obj) noexcept;

    //    bool eventFilter(QObject *watched, QEvent *event) override ;

private slots:
    void kill() noexcept;

private:
    static QAtomicPointer<Probe> s_probeInstance;
    std::set<const QObject *> knownObjects_;

    static Probe *probeInstance() noexcept;

    void discoverObject(QObject *obj) noexcept;
    void findObjectsFromCoreApp() noexcept;

    void installEventFilter() noexcept;
    bool isIternalObjectCreated(QObject *obj) const noexcept;
};
} // namespace QtAda::core
