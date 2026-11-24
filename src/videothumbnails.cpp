#include "videothumbnails.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QMediaPlayer>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVideoSink>

VideoThumbnails::VideoThumbnails(const QString &cacheDir, QObject *parent) : QObject(parent), m_dir(cacheDir) {
    if (m_dir.isEmpty()) m_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/thumbnails";
    QDir().mkpath(m_dir);
    m_timeout = new QTimer(this);
    m_timeout->setSingleShot(true);
    m_timeout->setInterval(10000);
    connect(m_timeout, &QTimer::timeout, this, [this] { finish(false); });
}

VideoThumbnails::~VideoThumbnails() {
    if (m_player) m_player->stop();
}

QString VideoThumbnails::cachePath(const QString &videoUrl) const {
    const QFileInfo fi(QUrl(videoUrl).toLocalFile());
    // The key includes size and modification time, so a replaced file gets a new thumbnail.
    const QByteArray key = (fi.absoluteFilePath() + "|" + QString::number(fi.size()) + "|" + QString::number(fi.lastModified().toSecsSinceEpoch())).toUtf8();
    return m_dir + "/" + QCryptographicHash::hash(key, QCryptographicHash::Sha1).toHex() + ".png";
}

QString VideoThumbnails::thumbnail(const QString &videoUrl) {
    const QString local = QUrl(videoUrl).toLocalFile();
    if (local.isEmpty() || !QFileInfo::exists(local)) return {};
    const QString path = cachePath(videoUrl);
    if (QFileInfo::exists(path)) return QUrl::fromLocalFile(path).toString();
    if (!m_failed.contains(videoUrl) && videoUrl != m_current && !m_queue.contains(videoUrl)) {
        m_queue.enqueue(videoUrl);
        QTimer::singleShot(0, this, &VideoThumbnails::startNext);
    }
    return {};
}

void VideoThumbnails::forget(const QString &videoUrl) {
    QFile::remove(cachePath(videoUrl));
    m_failed.remove(videoUrl);
    m_queue.removeAll(videoUrl);
}

void VideoThumbnails::startNext() {
    if (m_player || m_queue.isEmpty()) return;
    m_current = m_queue.dequeue();

    m_player = new QMediaPlayer(this);          // no audio output is set: it plays silently
    m_sink = new QVideoSink(this);
    m_player->setVideoSink(m_sink);
    connect(m_sink, &QVideoSink::videoFrameChanged, this, &VideoThumbnails::onFrame);
    connect(m_player, &QMediaPlayer::errorOccurred, this, [this] { finish(false); });
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus s) {
        if (s == QMediaPlayer::InvalidMedia) { finish(false); return; }
        if (s == QMediaPlayer::LoadedMedia) {
            // About one second in (the first frame is often black); short clips use the start.
            m_targetMs = m_player->duration() > 2500 ? 1000 : 0;
            m_player->setPosition(m_targetMs);
            m_player->play();
        }
    });
    m_timeout->start();
    m_player->setSource(QUrl(m_current));
}

void VideoThumbnails::onFrame(const QVideoFrame &frame) {
    if (!m_player || m_current.isEmpty() || !frame.isValid()) return;
    if (m_targetMs > 0 && frame.startTime() >= 0 && frame.startTime() < (m_targetMs - 150) * 1000) return;   // wait for the right moment
    QImage img = frame.toImage();
    if (img.isNull()) return;
    img = img.scaled(480, 270, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    finish(img.save(cachePath(m_current), "PNG"));
}

void VideoThumbnails::finish(bool ok) {
    if (m_current.isEmpty()) return;
    m_timeout->stop();
    const QString done = m_current;
    m_current.clear();
    if (m_player) { m_player->stop(); m_player->deleteLater(); }
    if (m_sink) m_sink->deleteLater();
    if (ok) { ++m_revision; emit changed(); }
    else m_failed.insert(done);
    QTimer::singleShot(0, this, &VideoThumbnails::startNext);
}
