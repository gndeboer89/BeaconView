#include <QtTest>
#include <QDir>
#include <QStandardPaths>
#include "songfilter.h"
#include "songlibrary.h"

class TstSongFilter : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("BeaconViewTest");
        QCoreApplication::setApplicationName("tst_songfilter");
        QStandardPaths::setTestModeEnabled(true);
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).removeRecursively();
    }
    void sortsAndSearches() {
        SongLibrary lib;
        lib.add("Zegen ons", "12", "Heer zegen ons\n\nin Jezus naam");
        lib.add("Genade", "", "Genade, zo groot\nGods liefde");
        lib.add("amazing grace", "", "How sweet the sound");
        SongFilter f;
        f.setSourceModel(&lib);
        QCOMPARE(f.total(), lib.rowCount());
        QCOMPARE(f.rowCount(), lib.rowCount());
        // sorted by title, not by when they were added; case does not matter
        QStringList titles;
        for (int i = 0; i < f.rowCount(); ++i) titles << f.data(f.index(i, 0), SongLibrary::TitleRole).toString();
        QStringList sorted = titles;
        std::sort(sorted.begin(), sorted.end(), [](const QString &a, const QString &b) { return a.compare(b, Qt::CaseInsensitive) < 0; });
        QCOMPARE(titles, sorted);

        f.setQuery("genade");                                  // title
        QCOMPARE(f.rowCount(), 1);
        f.setQuery("GÉNADE");                                  // accents and capitals do not matter
        QCOMPARE(f.rowCount(), 1);
        f.setQuery("sweet");                                   // a line of the lyrics
        QCOMPARE(f.rowCount(), 1);
        QCOMPARE(f.data(f.index(0, 0), SongLibrary::TitleRole).toString(), QStringLiteral("amazing grace"));
        f.setQuery("12");                                      // the song number
        QCOMPARE(f.rowCount(), 1);
        f.setQuery("heer jezus");                              // every word has to be somewhere
        QCOMPARE(f.rowCount(), 1);
        f.setQuery("heer genade");
        QCOMPARE(f.rowCount(), 0);
        QCOMPARE(f.total(), lib.rowCount());                   // the library itself is untouched
        f.setQuery("   ");
        QCOMPARE(f.rowCount(), lib.rowCount());
    }
    void mapsBackToTheLibrary() {
        SongLibrary lib;
        lib.add("Zzz laatste", "", "tekst");
        SongFilter f;
        f.setSourceModel(&lib);
        f.setQuery("laatste");
        QCOMPARE(f.rowCount(), 1);
        const int row = f.sourceRow(0);
        QCOMPARE(lib.data(lib.index(row), SongLibrary::TitleRole).toString(), QStringLiteral("Zzz laatste"));
        QCOMPARE(f.sourceRow(5), -1);
    }
    void followsChangesInTheLibrary() {
        SongLibrary lib;
        SongFilter f;
        f.setSourceModel(&lib);
        QSignalSpy total(&f, &SongFilter::totalChanged);
        f.setQuery("nieuwlied");
        QCOMPARE(f.rowCount(), 0);
        lib.add("Nieuwlied", "", "tekst");
        QCOMPARE(f.rowCount(), 1);
        QVERIFY(total.count() >= 1);
        lib.remove(f.sourceRow(0));
        QCOMPARE(f.rowCount(), 0);
    }
};
QTEST_MAIN(TstSongFilter)
#include "tst_songfilter.moc"
