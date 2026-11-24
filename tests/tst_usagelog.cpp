#include <QtTest>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryDir>
#include "usagelog.h"

class TstUsageLog : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("BeaconViewTest");
        QCoreApplication::setApplicationName("tst_usagelog");
        QStandardPaths::setTestModeEnabled(true);
    }
    void init() { QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).removeRecursively(); }

    void countsPerSong() {
        UsageLog log;
        const QDateTime t(QDate(2026, 11, 8), QTime(10, 0));
        log.record("Amazing Grace", "4768151", "John Newton", "Zondag", t);
        log.record("Amazing grace", "4768151", "John Newton", "Zondag", t.addDays(7));     // same CCLI number, other spelling
        log.record("Genade", "", "", "Zondag", t);
        log.record("GENADE", "", "", "Zondag", t.addDays(14));                              // no number: found by title
        log.record("Genade", "", "", "Avond", t.addDays(14));
        log.record("  ", "1", "", "", t);                                                    // no title: not recorded
        const QVariantList s = log.summary(QDate(), QDate());
        QCOMPARE(s.size(), 2);
        QCOMPARE(s[0].toMap()["title"].toString(), QStringLiteral("Genade"));               // the most used first
        QCOMPARE(s[0].toMap()["count"].toInt(), 3);
        QCOMPARE(s[1].toMap()["ccli"].toString(), QStringLiteral("4768151"));
        QCOMPARE(s[1].toMap()["count"].toInt(), 2);
        QCOMPARE(s[1].toMap()["last"].toString(), QStringLiteral("2026-11-15"));
        QCOMPARE(s[1].toMap()["title"].toString(), QStringLiteral("Amazing grace"));        // the most recent spelling
    }
    void periods() {
        UsageLog log;
        log.record("A", "1", "", "", QDateTime(QDate(2026, 10, 31), QTime(23, 59)));
        log.record("A", "1", "", "", QDateTime(QDate(2026, 11, 1), QTime(0, 0)));
        log.record("A", "1", "", "", QDateTime(QDate(2026, 11, 30), QTime(23, 59)));
        log.record("B", "2", "", "", QDateTime(QDate(2026, 12, 1), QTime(0, 0)));
        log.record("C", "3", "", "", QDateTime(QDate(2025, 6, 1), QTime(12, 0)));
        auto count = [&](const QString &p) {
            QDate from, to; UsageLog::range(p, QDate(2026, 11, 15), &from, &to);
            int n = 0; for (const auto &v : log.summary(from, to)) n += v.toMap()["count"].toInt(); return n;
        };
        QCOMPARE(count("month"), 2);        // 1 to 30 November
        QCOMPARE(count("lastmonth"), 1);    // October
        QCOMPARE(count("year"), 4);         // 2026
        QCOMPARE(count("all"), 5);
        QDate f, t; UsageLog::range("lastmonth", QDate(2026, 1, 15), &f, &t);   // across the new year
        QCOMPARE(f, QDate(2025, 12, 1)); QCOMPARE(t, QDate(2025, 12, 31));
    }
    void csvIsSafeAndReadable() {
        UsageLog log;
        log.record("Lied; met \"puntkomma\"", "99", "Jan; Piet", "", QDateTime(QDate(2026, 11, 8), QTime(10, 0)));
        log.record("=SOM(A1)", "", "", "", QDateTime(QDate(2026, 11, 8), QTime(10, 0)));
        const QString csv = log.csv(QDate(), QDate());
        QVERIFY(csv.startsWith("Titel;CCLI-nummer;Auteur;Aantal keer;Laatst gebruikt\r\n"));
        QVERIFY(csv.contains("\"Lied; met \"\"puntkomma\"\"\";99;\"Jan; Piet\";1;2026-11-08"));
        QVERIFY(csv.contains("'=SOM(A1)"));                              // never a formula in a spreadsheet
        QTemporaryDir dir;
        const QString file = dir.filePath("rapport.csv");
        QVERIFY(log.exportCsv(QUrl::fromLocalFile(file), "all"));
        QFile f(file); QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray data = f.readAll();
        QVERIFY(data.startsWith("\xEF\xBB\xBF"));                        // so that Excel shows accents correctly
        QVERIFY(!log.exportCsv(QUrl::fromLocalFile(dir.filePath("niet/bestaand/x.csv")), "all"));
    }
    void survivesRestartAndClears() {
        { UsageLog log; log.record("Lied", "5", "A", "Zondag"); }
        UsageLog again;
        QCOMPARE(again.count(), 1);
        QCOMPARE(again.uses("all"), 1);
        QSignalSpy spy(&again, &UsageLog::changed);
        again.clear();
        QCOMPARE(again.count(), 0);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(UsageLog().count(), 0);
        QVERIFY(!again.periodText("month").isEmpty());
        QCOMPARE(again.periodText("all"), QStringLiteral("alles"));
    }
};
QTEST_GUILESS_MAIN(TstUsageLog)
#include "tst_usagelog.moc"
