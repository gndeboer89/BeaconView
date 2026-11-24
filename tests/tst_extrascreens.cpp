#include <QtTest>
#include <QSettings>
#include <QStandardPaths>
#include "extrascreens.h"

class TstExtraScreens : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("BeaconViewTest");
        QCoreApplication::setApplicationName("tst_extrascreens");
        QStandardPaths::setTestModeEnabled(true);
    }
    void init() { QSettings().clear(); }
    void addsWithDefaults() {
        ExtraScreens s;
        QCOMPARE(s.rowCount(), 0);
        QCOMPARE(s.add("onzin"), -1);
        QCOMPARE(s.add("stage"), 0);
        QCOMPARE(s.add("subtitles"), 1);
        QCOMPARE(s.add("subtitles"), 2);
        QCOMPARE(s.get(0, "name").toString(), QStringLiteral("Stage-scherm"));
        QCOMPARE(s.get(1, "name").toString(), QStringLiteral("Ondertitels"));
        QCOMPARE(s.get(2, "name").toString(), QStringLiteral("Ondertitels 2"));
        QCOMPARE(s.get(1, "chromaColor").toString(), QStringLiteral("#00b140"));   // broadcast green
        QVERIFY(s.get(0, "showNext").toBool());
        QCOMPARE(s.get(1, "screenName").toString(), QString());                   // a loose window until a monitor is chosen
        QVERIFY(!s.get(0, "on").toBool());
        QVERIFY(s.get(0, "id").toInt() != s.get(1, "id").toInt());
        // QML reads the same values through the role names
        QCOMPARE(s.data(s.index(1), s.roleNames().key("chromaColor")).toString(), QStringLiteral("#00b140"));
    }
    void setValidates() {
        ExtraScreens s;
        s.add("subtitles");
        QSignalSpy spy(&s, &QAbstractItemModel::dataChanged);
        s.set(0, "scale", 9);   QCOMPARE(s.get(0, "scale").toDouble(), 2.0);     // limited
        s.set(0, "scale", 0.1); QCOMPARE(s.get(0, "scale").toDouble(), 0.5);
        s.set(0, "position", "ergens"); QCOMPARE(s.get(0, "position").toString(), QStringLiteral("bottom"));
        s.set(0, "position", "top");    QCOMPARE(s.get(0, "position").toString(), QStringLiteral("top"));
        const int before = spy.count();
        s.set(0, "position", "top");                                              // no change, no signal
        QCOMPARE(spy.count(), before);
        const int id = s.get(0, "id").toInt();
        s.set(0, "id", 99); s.set(0, "kind", "stage"); s.set(0, "bestaatniet", 1);
        QCOMPARE(s.get(0, "id").toInt(), id);                                     // id and kind cannot change
        QCOMPARE(s.get(0, "kind").toString(), QStringLiteral("subtitles"));
        s.set(5, "name", "x");                                                    // out of range
    }
    void survivesRestartButNotOn() {
        {
            ExtraScreens s;
            s.add("stage"); s.add("subtitles");
            s.set(0, "name", "Podium"); s.set(0, "screenName", "HDMI-2"); s.set(0, "scale", 1.4);
            s.set(1, "chromaColor", "#0047bb"); s.set(1, "on", true);
        }
        ExtraScreens again;
        QCOMPARE(again.rowCount(), 2);
        QCOMPARE(again.get(0, "name").toString(), QStringLiteral("Podium"));
        QCOMPARE(again.get(0, "screenName").toString(), QStringLiteral("HDMI-2"));
        QCOMPARE(again.get(0, "scale").toDouble(), 1.4);
        QCOMPARE(again.get(1, "chromaColor").toString(), QStringLiteral("#0047bb"));
        QVERIFY(!again.get(1, "on").toBool());                                   // screens start closed
        const int id0 = again.get(0, "id").toInt();
        again.add("stage");
        QVERIFY(again.get(2, "id").toInt() > again.get(1, "id").toInt());        // ids stay unique after a restart
        QCOMPARE(again.get(0, "id").toInt(), id0);
    }
    void followsTheMainOutput() {
        ExtraScreens s;
        s.add("stage"); s.add("subtitles");
        s.set(1, "autoOpen", false);
        s.setOutputOn(true);
        QVERIFY(s.get(0, "on").toBool());
        QVERIFY(!s.get(1, "on").toBool());                                       // this one is opened by hand
        s.set(1, "on", true);
        QCOMPARE(s.openCount(), 2);
        s.setOutputOn(false);
        QVERIFY(!s.get(0, "on").toBool());
        QVERIFY(s.get(1, "on").toBool());
    }
    void removes() {
        ExtraScreens s;
        s.add("stage"); s.add("subtitles");
        QSignalSpy cnt(&s, &ExtraScreens::countChanged);
        s.remove(0);
        QCOMPARE(s.rowCount(), 1);
        QCOMPARE(s.get(0, "kind").toString(), QStringLiteral("subtitles"));
        QCOMPARE(cnt.count(), 1);
        s.remove(7);
        QCOMPARE(s.rowCount(), 1);
        ExtraScreens again;
        QCOMPARE(again.rowCount(), 1);
    }
};
QTEST_GUILESS_MAIN(TstExtraScreens)
#include "tst_extrascreens.moc"
