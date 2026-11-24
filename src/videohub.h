#pragma once
#include <QList>
#include <QObject>
#include <QPointer>
#include <QVideoFrame>
#include <QVideoSink>

// One video decoder, many screens: the media player draws into `source`, and every frame is
// forwarded to all registered targets (the output window and the preview). The sound plays only once.
// Exposed to QML as `videoHub`.
class VideoHub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVideoSink *source READ source CONSTANT)
public:
    explicit VideoHub(QObject *parent = nullptr) : QObject(parent), m_source(this) {
        connect(&m_source, &QVideoSink::videoFrameChanged, this, &VideoHub::forward);
    }
    QVideoSink *source() { return &m_source; }
    int targetCount() const { return m_targets.size(); }

    Q_INVOKABLE void addTarget(QVideoSink *sink) {
        if (!sink || m_targets.contains(sink)) return;
        m_targets.append(sink);
        if (m_source.videoFrame().isValid()) sink->setVideoFrame(m_source.videoFrame());   // show the current frame at once
    }
    Q_INVOKABLE void removeTarget(QVideoSink *sink) { m_targets.removeAll(sink); }

private:
    void forward(const QVideoFrame &frame) {
        m_targets.removeAll(nullptr);   // QPointer entries of destroyed sinks
        for (const auto &t : std::as_const(m_targets)) t->setVideoFrame(frame);
    }
    QVideoSink m_source;
    QList<QPointer<QVideoSink>> m_targets;
};
