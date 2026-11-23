#include "overlaycontroller.h"
#include <QElapsedTimer>
#include <QSettings>
#include <algorithm>

qint64 OverlayController::nowMs() {
    static QElapsedTimer t = [] { QElapsedTimer e; e.start(); return e; }();
    return t.elapsed();
}

OverlayController::OverlayController(QObject *parent, bool autoTick) : QObject(parent), m_autoTick(autoTick) {
    m_messages = QSettings().value("overlayMessages", QStringList{
        QStringLiteral("Welkom! Fijn dat u er bent."),
        QStringLiteral("Collecte: u kunt ook digitaal geven. Zie de gegevens op het scherm."),
        QStringLiteral("De kinderopvang vraagt: wilt u zich melden bij de balie?")}).toStringList();
    m_ticker.setInterval(200);
    connect(&m_ticker, &QTimer::timeout, this, [this] { tick(nowMs()); });
}

QString OverlayController::formatTime(int s) {
    s = qMax(0, s);
    const int h = s / 3600, m = (s % 3600) / 60, sec = s % 60;
    return h > 0 ? QStringLiteral("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'))
                 : QStringLiteral("%1:%2").arg(m, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
}

// ---- timer ---------------------------------------------------------------------------

int OverlayController::compute(qint64 now) const {
    const qint64 e = elapsed(now);
    if (m_mode == Countdown) {
        const qint64 left = qint64(m_duration) * 1000 - e;
        return left <= 0 ? 0 : int((left + 999) / 1000);   // round up: 00:01 stays until the last moment
    }
    return int(e / 1000);
}

void OverlayController::tick(qint64 now) {
    if (!m_running) return;
    bool done = false;
    if (m_mode == Countdown && m_duration > 0 && elapsed(now) >= qint64(m_duration) * 1000) {
        m_running = false;
        m_finished = true;
        m_accum = qint64(m_duration) * 1000;
        m_ticker.stop();
        done = true;
    }
    const int d = compute(now);
    if (d != m_display || done) { m_display = d; emit timerChanged(); }
}

void OverlayController::startAt(qint64 now) {
    if (m_running) return;
    if (m_finished) { m_accum = 0; m_finished = false; }   // starting after the end restarts the countdown
    m_running = true;
    m_startedAt = now;
    m_display = compute(now);
    if (m_autoTick) m_ticker.start();
    emit timerChanged();
}

void OverlayController::pauseAt(qint64 now) {
    if (!m_running) return;
    m_accum = elapsed(now);
    m_running = false;
    m_display = compute(now);   // show the exact value at the moment of pausing
    m_ticker.stop();
    emit timerChanged();
}

void OverlayController::start() { startAt(nowMs()); }
void OverlayController::pause() { pauseAt(nowMs()); }
void OverlayController::toggleRunning() { m_running ? pause() : start(); }

void OverlayController::reset() {
    m_running = false;
    m_finished = false;
    m_accum = 0;
    m_ticker.stop();
    m_display = compute(0);
    emit timerChanged();
}

void OverlayController::setTimerMode(int m) {
    m = (m == Stopwatch) ? Stopwatch : Countdown;
    if (m == m_mode) return;
    m_mode = m;
    reset();
}

void OverlayController::setTimerDuration(int seconds) {
    seconds = std::clamp(seconds, 0, 99 * 3600);
    if (seconds == m_duration) return;
    m_duration = seconds;
    if (!m_running) reset(); else emit timerChanged();
}

void OverlayController::setTimerShown(bool b) { if (b != m_timerShown) { m_timerShown = b; emit timerChanged(); } }
void OverlayController::setTimerCorner(bool b) { if (b != m_corner) { m_corner = b; emit timerChanged(); } }
void OverlayController::setTimerLabel(const QString &l) { if (l != m_label) { m_label = l; emit timerChanged(); } }
void OverlayController::setClockShown(bool b) { if (b != m_clock) { m_clock = b; emit clockShownChanged(); } }

// ---- messages ------------------------------------------------------------------------

void OverlayController::saveMessages() const { QSettings().setValue("overlayMessages", m_messages); }

void OverlayController::addMessage(const QString &text) {
    if (text.trimmed().isEmpty()) return;
    m_messages << text.trimmed();
    saveMessages();
    emit messagesChanged();
}

void OverlayController::updateMessage(int i, const QString &text) {
    if (i < 0 || i >= m_messages.size() || text.trimmed().isEmpty()) return;
    const bool live = m_msgShown && m_msgText == m_messages[i];
    m_messages[i] = text.trimmed();
    if (live) { m_msgText = m_messages[i]; emit messageChanged(); }
    saveMessages();
    emit messagesChanged();
}

void OverlayController::removeMessage(int i) {
    if (i < 0 || i >= m_messages.size()) return;
    const bool live = m_msgShown && m_msgText == m_messages[i];
    m_messages.removeAt(i);
    saveMessages();
    emit messagesChanged();
    if (live) hideMessage();
}

void OverlayController::toggleMessage(int i) {
    if (i < 0 || i >= m_messages.size()) return;
    if (m_msgShown && m_msgText == m_messages[i]) { hideMessage(); return; }
    m_msgText = m_messages[i];
    m_msgShown = true;
    emit messageChanged();
}

void OverlayController::hideMessage() {
    if (!m_msgShown) return;
    m_msgShown = false;   // text is kept so the bar can fade out
    emit messageChanged();
}
