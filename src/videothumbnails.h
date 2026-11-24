#pragma once
#include <QObject>
#include <QQueue>
#include <QSet>
#include <QPointer>
#include <QVideoFrame>

class QMediaPlayer;
class QVideoSink;
class QTimer;

// Thumbnails of video files (a frame from about one second in), made one at a time in the background and
// cached on disk. If a video cannot be decoded it simply has no thumbnail (the UI shows a play symbol).
// Exposed to QML as `videoThumbs`.
class VideoThumbnails : public QObject {
    Q_OBJECT
    Q_PROPERTY(int revision READ revision NOTIFY changed)   // bind to this so thumbnail() is asked again when one arrives
public:
    explicit VideoThumbnails(const QString &cacheDir = QString(), QObject *parent = nullptr);
    ~VideoThumbnails() override;

    int revision() const { return m_revision; }

    // The cached thumbnail as a file URL, or "" — then it is generated in the background and `changed` is emitted.
    Q_INVOKABLE QString thumbnail(const QString &videoUrl);
    Q_INVOKABLE void forget(const QString &videoUrl);   // removes the cached thumbnail

signals:
    void changed();

private:
    QString cachePath(const QString &videoUrl) const;
    void startNext();
    void onFrame(const QVideoFrame &frame);
    void finish(bool ok);

    QString m_dir;
    int m_revision = 0;
    QQueue<QString> m_queue;
    QSet<QString> m_failed;
    QString m_current;
    qint64 m_targetMs = 0;
    QPointer<QMediaPlayer> m_player;
    QPointer<QVideoSink> m_sink;
    QTimer *m_timeout = nullptr;
};
