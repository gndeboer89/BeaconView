#include <QtTest>
#include "overlaycontroller.h"

class TstOverlays : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("BeaconViewTest");
        QCoreApplication::setApplicationName("tst_overlays");
        QSettings().clear();
    }
    void formatting() {
        QCOMPARE(OverlayController::formatTime(0), QStringLiteral("00:00"));
        QCOMPARE(OverlayController::formatTime(65), QStringLiteral("01:05"));
        QCOMPARE(OverlayController::formatTime(3600 + 5 * 60 + 9), QStringLiteral("1:05:09"));
        QCOMPARE(OverlayController::formatTime(-4), QStringLiteral("00:00"));
    }
    void countdown() {
        OverlayController c(nullptr, false);
        c.setTimerDuration(10);
        QCOMPARE(c.timerSeconds(), 10);
        c.startAt(1000);
        c.tick(1000); QCOMPARE(c.timerSeconds(), 10);
        c.tick(1001); QCOMPARE(c.timerSeconds(), 10);   // rounds up, 9.999s left still shows 10
        c.tick(3500); QCOMPARE(c.timerSeconds(), 8);
        c.pauseAt(4000);                                // 3s elapsed
        QVERIFY(!c.timerRunning());
        c.tick(60000); QCOMPARE(c.timerSeconds(), 7);   // paused: no change
        c.startAt(100000);
        c.tick(106900); QCOMPARE(c.timerSeconds(), 1);  // 3s + 6.9s elapsed
        QVERIFY(!c.timerFinished());
        c.tick(107000);
        QCOMPARE(c.timerSeconds(), 0);
        QVERIFY(c.timerFinished());
        QVERIFY(!c.timerRunning());
        c.startAt(200000);                              // starting after the end restarts
        QVERIFY(c.timerRunning() && !c.timerFinished());
        QCOMPARE(c.timerSeconds(), 10);
        c.reset();
        QCOMPARE(c.timerSeconds(), 10);
        QVERIFY(!c.timerRunning());
    }
    void stopwatch() {
        OverlayController c(nullptr, false);
        c.setTimerMode(OverlayController::Stopwatch);
        QCOMPARE(c.timerSeconds(), 0);
        c.startAt(500);
        c.tick(5600); QCOMPARE(c.timerSeconds(), 5);
        c.pauseAt(8000); QCOMPARE(c.timerSeconds(), 7);   // 7.5s elapsed
        c.startAt(9000);
        c.tick(11000); QCOMPARE(c.timerSeconds(), 9);   // 7.5s + 2s
        QVERIFY(!c.timerFinished());
    }
    void zeroDurationNeverFinishes() {
        OverlayController c(nullptr, false);
        c.setTimerDuration(0);
        c.startAt(0); c.tick(5000);
        QVERIFY(!c.timerFinished());
    }
    void messages() {
        OverlayController c(nullptr, false);
        const int n = c.messages().size();
        c.addMessage("  Hallo  "); c.addMessage("   ");
        QCOMPARE(c.messages().size(), n + 1);
        QCOMPARE(c.messages().last(), QStringLiteral("Hallo"));
        c.toggleMessage(n);
        QVERIFY(c.messageShown()); QCOMPARE(c.messageText(), QStringLiteral("Hallo"));
        c.updateMessage(n, "Hoi");
        QCOMPARE(c.messageText(), QStringLiteral("Hoi"));   // live text follows the edit
        c.toggleMessage(n);
        QVERIFY(!c.messageShown());
        QCOMPARE(c.messageText(), QStringLiteral("Hoi"));   // kept for fade-out
        c.toggleMessage(n); c.removeMessage(n);
        QVERIFY(!c.messageShown());                          // removing the live one hides it
        QCOMPARE(c.messages().size(), n);
        OverlayController again(nullptr, false);             // persisted
        QCOMPARE(again.messages().size(), n);
    }
};
QTEST_GUILESS_MAIN(TstOverlays)
#include "tst_overlays.moc"
