#include <QtTest>
#include "bible.h"

class TstBible : public QObject {
    Q_OBJECT
    Bible bible;
private slots:
    void data() {
        QCOMPARE(bible.bookCount(), 66);
        QCOMPARE(bible.chapterCount(18), 150);  // Psalmen
        QCOMPARE(bible.verseText(18, 23, 1), QStringLiteral("Een psalm van David. De HEERE is mijn Herder, mij zal niets ontbreken."));
    }
    void parseVariants() {
        for (const char *s : {"Joh 3:16", "joh. 3,16", "Johannes 3:16", "JOH 3 : 16"}) {
            const auto p = bible.parse(QString::fromUtf8(s));
            QVERIFY2(p.ok(), s);
            QCOMPARE(bible.label(p), QStringLiteral("Johannes 3:16"));
        }
        QCOMPARE(bible.label(bible.parse("1 Kor 13:4-7")), QStringLiteral("1 Korinthe 13:4-7"));
        QCOMPARE(bible.label(bible.parse("1kor 13")), QStringLiteral("1 Korinthe 13"));
        QCOMPARE(bible.label(bible.parse("Psalm 23:1-3,5")), QStringLiteral("Psalm 23:1-3,5"));
        QCOMPARE(bible.label(bible.parse("Psalm 23")), QStringLiteral("Psalm 23"));
        QCOMPARE(bible.label(bible.parse("Ez 1:1")), QStringLiteral("Ezechiël 1:1"));          // alias beats ambiguous prefix
        QCOMPARE(bible.label(bible.parse("Mattheüs 5:3")), QStringLiteral("Mattheüs 5:3"));
        QCOMPARE(bible.label(bible.parse("matheus 5:3")), QString());                        // unknown spelling
    }
    void parseErrors() {
        QVERIFY(!bible.parse("Psalm 151").ok());
        QVERIFY(!bible.parse("Psalm 23:7").ok());
        QVERIFY(!bible.parse("Psalm 23:3-1").ok());
        QVERIFY(!bible.parse("Xyz 1").ok());
        QVERIFY(bible.parse("Johannes").bookFound);
        QVERIFY(!bible.parse("Johannes").ok());
        QVERIFY(!bible.parse("").ok());
    }
    void slides() {
        const auto s = bible.slides(bible.parse("Psalm 23:1-5"), 2, true);
        QCOMPARE(s.size(), 3);
        QCOMPARE(s[0].reference, QStringLiteral("Psalm 23:1-2"));
        QCOMPARE(s[2].reference, QStringLiteral("Psalm 23:5"));
        QVERIFY(s[0].text.startsWith("1 Een psalm van David."));
        QVERIFY(bible.slides(bible.parse("Psalm 23:1"), 2, false)[0].text.startsWith("Een psalm"));
    }
    void search() {
        const auto r = bible.search("zeer stille wateren");
        QVERIFY(!r.isEmpty());
        QCOMPARE(r[0].toMap()["ref"].toString(), QStringLiteral("Psalm 23:2"));
        QVERIFY(bible.search("ab").isEmpty());
    }
};
QTEST_APPLESS_MAIN(TstBible)
#include "tst_bible.moc"
