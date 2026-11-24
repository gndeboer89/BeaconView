#include "singleinstance.h"
#include <QCoreApplication>
#include <QDir>
#include <QLocalSocket>
#include <QRegularExpression>
#include <QThread>

SingleInstance::SingleInstance(const QString &key, QObject *parent) : QObject(parent) {
    QString user = qEnvironmentVariable("USERNAME", qEnvironmentVariable("USER"));
    const QString name = key + "-" + user.replace(QRegularExpression("[^A-Za-z0-9_]"), "_");

    m_lock = std::make_unique<QLockFile>(QDir::temp().filePath(name + ".lock"));
    m_lock->setStaleLockTime(0);          // a lock left behind by a crashed copy is detected by its process id
    if (m_lock->tryLock(0)) {
        m_primary = true;
        m_server = std::make_unique<QLocalServer>();
        QLocalServer::removeServer(name);   // a socket left behind by a crash
        m_server->setSocketOptions(QLocalServer::UserAccessOption);
        m_server->listen(name);
        connect(m_server.get(), &QLocalServer::newConnection, this, [this] {
            while (QLocalSocket *c = m_server->nextPendingConnection()) {
                c->deleteLater();
                emit activated();
            }
        });
        return;
    }
    // Someone else is running (or still starting up and not listening yet): ask it to come to the front.
    for (int attempt = 0; attempt < 20; ++attempt) {
        QLocalSocket s;
        s.connectToServer(name);
        if (s.waitForConnected(200)) {
            s.write("activate\n");
            s.waitForBytesWritten(500);
            s.disconnectFromServer();
            return;
        }
        QThread::msleep(100);
    }
}
