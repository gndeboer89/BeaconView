#pragma once
#include <QLocalServer>
#include <QLockFile>
#include <QObject>
#include <memory>

// Makes sure the program runs once per user. The first copy is the "primary"; a second copy tells the first to come to the
// front (activated()) and then should quit. A lock file decides who is first (also when two start at the same moment),
// a local socket carries the message.
class SingleInstance : public QObject {
    Q_OBJECT
public:
    explicit SingleInstance(const QString &key, QObject *parent = nullptr);
    bool isPrimary() const { return m_primary; }
signals:
    void activated();     // another copy was started: bring the window to the front

private:
    bool m_primary = false;
    std::unique_ptr<QLockFile> m_lock;
    std::unique_ptr<QLocalServer> m_server;
};
