#pragma once
#include <QObject>
#include <QStringList>
#include <QTimer>

// Everything that is drawn on top of the slides: a countdown/stopwatch timer, a clock
// and message bars ("meldingen"). Exposed to QML as `presenter.overlays`.
class OverlayController : public QObject {
    Q_OBJECT
    // messages
    Q_PROPERTY(QStringList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(QString messageText READ messageText NOTIFY messageChanged)
    Q_PROPERTY(bool messageShown READ messageShown NOTIFY messageChanged)
    Q_PROPERTY(bool clockShown READ clockShown WRITE setClockShown NOTIFY clockShownChanged)
    // timer: mode 0 = countdown, 1 = stopwatch
    Q_PROPERTY(int timerMode READ timerMode WRITE setTimerMode NOTIFY timerChanged)
    Q_PROPERTY(int timerDuration READ timerDuration WRITE setTimerDuration NOTIFY timerChanged)  // countdown start, seconds
    Q_PROPERTY(int timerSeconds READ timerSeconds NOTIFY timerChanged)                          // value to display
    Q_PROPERTY(QString timerText READ timerText NOTIFY timerChanged)                            // "05:00" / "1:05:00"
    Q_PROPERTY(bool timerRunning READ timerRunning NOTIFY timerChanged)
    Q_PROPERTY(bool timerFinished READ timerFinished NOTIFY timerChanged)
    Q_PROPERTY(bool timerShown READ timerShown WRITE setTimerShown NOTIFY timerChanged)
    Q_PROPERTY(bool timerCorner READ timerCorner WRITE setTimerCorner NOTIFY timerChanged)
    Q_PROPERTY(QString timerLabel READ timerLabel WRITE setTimerLabel NOTIFY timerChanged)
public:
    enum Mode { Countdown = 0, Stopwatch = 1 };
    explicit OverlayController(QObject *parent = nullptr, bool autoTick = true);

    QStringList messages() const { return m_messages; }
    QString messageText() const { return m_msgText; }
    bool messageShown() const { return m_msgShown; }
    bool clockShown() const { return m_clock; }
    void setClockShown(bool b);

    int timerMode() const { return m_mode; }
    void setTimerMode(int m);
    int timerDuration() const { return m_duration; }
    void setTimerDuration(int seconds);
    int timerSeconds() const { return m_display; }
    QString timerText() const { return formatTime(m_display); }
    bool timerRunning() const { return m_running; }
    bool timerFinished() const { return m_finished; }
    bool timerShown() const { return m_timerShown; }
    void setTimerShown(bool b);
    bool timerCorner() const { return m_corner; }
    void setTimerCorner(bool b);
    QString timerLabel() const { return m_label; }
    void setTimerLabel(const QString &l);

    static QString formatTime(int seconds);

    // Timer control. The *At variants take an explicit monotonic time in ms so they can be tested.
    void startAt(qint64 nowMs);
    void pauseAt(qint64 nowMs);
    void tick(qint64 nowMs);
    Q_INVOKABLE void start();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void toggleRunning();
    Q_INVOKABLE void reset();

    // Messages
    Q_INVOKABLE void addMessage(const QString &text);
    Q_INVOKABLE void updateMessage(int i, const QString &text);
    Q_INVOKABLE void removeMessage(int i);
    Q_INVOKABLE void toggleMessage(int i);
    Q_INVOKABLE void hideMessage();

signals:
    void messagesChanged();
    void messageChanged();
    void clockShownChanged();
    void timerChanged();

private:
    qint64 elapsed(qint64 nowMs) const { return m_accum + (m_running ? nowMs - m_startedAt : 0); }
    int compute(qint64 nowMs) const;
    void saveMessages() const;
    static qint64 nowMs();

    QStringList m_messages;
    QString m_msgText;
    bool m_msgShown = false, m_clock = false;

    int m_mode = Countdown, m_duration = 300, m_display = 300;
    qint64 m_accum = 0, m_startedAt = 0;
    bool m_running = false, m_finished = false, m_timerShown = false, m_corner = false;
    QString m_label = QStringLiteral("De dienst begint over");
    QTimer m_ticker;
    bool m_autoTick;
};
