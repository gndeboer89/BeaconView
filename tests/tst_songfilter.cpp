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
    void categoriesAndTags() {
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).removeRecursively();
        QStringList seenMeta;
        {
            SongLibrary lib;
            QSignalSpy meta(&lib, &SongLibrary::metaChanged);
            lib.add("Stille nacht", "", "t", "", "Liedboek", {"Kerst", "#rust"});
            lib.add("Ere zij God", "", "t", "", "liedboek ", {"kerst", "  Advent "});     // same bundle and tag, other capitals
            lib.add("Opwekking 1", "1", "t", "", "Opwekking", {"Pasen", "kerst", "Pasen"});   // a tag twice is one tag
            lib.add("Zonder niets", "", "t");
            QVERIFY(meta.count() >= 4);
            QCOMPARE(lib.categories(), (QStringList{"Liedboek", "Opwekking"}));             // sorted, the first spelling kept
            QCOMPARE(lib.tags(), (QStringList{"Advent", "Kerst", "Pasen", "rust"}));
            const int r = lib.rowCount();
            QCOMPARE(lib.data(lib.index(r - 4), SongLibrary::TagsRole).toStringList(), (QStringList{"Kerst", "rust"}));    // # is dropped
            QCOMPARE(lib.data(lib.index(r - 3), SongLibrary::CategoryRole).toString(), QStringLiteral("Liedboek"));
            QCOMPARE(lib.data(lib.index(r - 2), SongLibrary::TagsRole).toStringList(), (QStringList{"Pasen", "Kerst"}));    // given order kept
            QCOMPARE(SongLibrary::parseTags("kerst, Pasen;  advent\n"), (QStringList{"kerst", "Pasen", "advent"}));
        }
        SongLibrary lib;                                                                      // saved and loaded
        QCOMPARE(lib.categories(), (QStringList{"Liedboek", "Opwekking"}));
        SongFilter f;
        f.setSourceModel(&lib);
        const int all = f.rowCount();
        f.setCategory("liedboek");
        QCOMPARE(f.rowCount(), 2);
        f.setCategory("");
        f.setTag("KERST");                                                                    // a song can have more than one tag
        QCOMPARE(f.rowCount(), 3);
        f.setTag("pasen");
        QCOMPARE(f.rowCount(), 1);
        f.setTag("kerst"); f.setCategory("Opwekking");                                         // both filters together
        QCOMPARE(f.rowCount(), 1);
        f.setCategory(""); f.setTag("");
        QCOMPARE(f.rowCount(), all);
        f.setQuery("advent");                                                                  // the search also finds tags and bundles
        QCOMPARE(f.rowCount(), 1);
        f.setQuery("liedboek");
        QCOMPARE(f.rowCount(), 2);
        f.setQuery("");
        // editing and giving to several songs at once
        const int row = f.sourceRow(0);
        lib.update(row, "Titel", "", "tekst", "", "Nieuwe bundel", {"nieuw"});
        QVERIFY(lib.categories().contains("Nieuwe bundel"));
        QVERIFY(lib.tags().contains("nieuw"));
        lib.setCategory({0, 1}, "Hillsong");
        lib.addTag({0, 1, 2}, "Pinksteren");
        QCOMPARE(lib.data(lib.index(0), SongLibrary::CategoryRole).toString(), QStringLiteral("Hillsong"));
        QVERIFY(lib.data(lib.index(2), SongLibrary::TagsRole).toStringList().contains("Pinksteren"));
        lib.addTag({0}, "pinksteren");
        QCOMPARE(lib.data(lib.index(0), SongLibrary::TagsRole).toStringList().count("Pinksteren"), 1);
        lib.setCategory({99}, "x");                                                            // out of range: nothing happens
        QVERIFY(!lib.categories().contains("x"));
        // a bundle that no song uses any more is gone from the list
        for (int i = lib.rowCount() - 1; i >= 0; --i) lib.setCategory({i}, "");
        QVERIFY(lib.categories().isEmpty());
    }
};
QTEST_MAIN(TstSongFilter)
#include "tst_songfilter.moc"
