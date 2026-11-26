#include <QtTest>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTemporaryDir>
#include "songdatabase.h"
#include "songlibrary.h"

static SongDatabase::Row row(const QString &title, const QString &lyrics = "tekst", const QStringList &tags = {}) {
    SongDatabase::Row r;
    r.title = title; r.lyrics = lyrics; r.tags = tags;
    return r;
}

class TstSongDatabase : public QObject {
    Q_OBJECT
    QTemporaryDir tmp;
    QString dbPath(const QString &name = "songs.db") { return tmp.filePath(name); }
    // A "folder" that is really a file: nothing can be made under it.
    QString blockedPath() {
        QFile f(tmp.filePath("blocker")); f.open(QIODevice::WriteOnly); f.write("x"); f.close();
        return tmp.filePath("blocker/songs.db");
    }
private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("BeaconViewTest");
        QCoreApplication::setApplicationName("tst_songdatabase");
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(tmp.isValid());
    }

    void storesSongsWithTagsInOrder() {
        SongDatabase db(dbPath("a.db"));
        QVERIFY(db.isOpen());
        QCOMPARE(db.count(), 0);
        SongDatabase::Row r = row("Genade", "[Refrein]\ngenade", {"Kerst", "Advent", "Pasen"});
        r.number = "12"; r.author = "Jan"; r.ccli = "123"; r.copyright = "(c)"; r.order = "Refrein, Refrein"; r.category = "Opwekking";
        const qint64 id = db.insert(r);
        QVERIFY(id > 0);
        const auto all = db.all();
        QCOMPARE(all.size(), 1);
        QCOMPARE(all[0].id, id);
        QCOMPARE(all[0].title, QStringLiteral("Genade"));
        QCOMPARE(all[0].order, QStringLiteral("Refrein, Refrein"));
        QCOMPARE(all[0].category, QStringLiteral("Opwekking"));
        QCOMPARE(all[0].tags, (QStringList{"Kerst", "Advent", "Pasen"}));     // the order of the tags is kept
        QCOMPARE(all[0].rev, qint64(1));
        // accents, quotes and line breaks survive
        const qint64 id2 = db.insert(row("Ere zij God, \"vrede\" op aarde; één", "regel 1\nregel 2\n\n[Brug]\në 'x'"));
        QCOMPARE(db.all().last().lyrics, QStringLiteral("regel 1\nregel 2\n\n[Brug]\në 'x'"));
        QVERIFY(db.remove(id2));
        QCOMPARE(db.count(), 1);
        QVERIFY(db.remove(id));                                                // its tags go with it
        QCOMPARE(db.count(), 0);
    }
    void revisionsAndConflicts() {
        SongDatabase db(dbPath("b.db"));
        SongDatabase::Row r = row("Lied");
        r.id = db.insert(r);
        qint64 rev = 0;
        r.title = "Lied 2";
        QCOMPARE(db.update(r, 1, &rev), SongDatabase::Updated);
        QCOMPARE(rev, qint64(2));
        QCOMPARE(db.revisionOf(r.id), qint64(2));
        r.title = "Lied 3";
        QCOMPARE(db.update(r, 1), SongDatabase::Conflict);                     // based on an old version: not saved
        QCOMPARE(db.all()[0].title, QStringLiteral("Lied 2"));
        QCOMPARE(db.update(r, -1, &rev), SongDatabase::Updated);               // "save anyway"
        QCOMPARE(rev, qint64(3));
        SongDatabase::Row ghost = row("Spook"); ghost.id = 9999;
        QCOMPARE(db.update(ghost, -1), SongDatabase::Gone);
        r.tags = {"a", "b"};
        QCOMPARE(db.update(r, 3), SongDatabase::Updated);
        r.tags = {"b"};
        QCOMPARE(db.update(r, 4), SongDatabase::Updated);
        QCOMPARE(db.all()[0].tags, (QStringList{"b"}));
    }
    void transactionsAreAllOrNothing() {
        SongDatabase db(dbPath("c.db"));
        QVERIFY(db.begin());
        db.insert(row("een")); db.insert(row("twee"));
        db.rollback();
        QCOMPARE(db.count(), 0);
        QVERIFY(db.begin());
        db.insert(row("een")); db.insert(row("twee"));
        QVERIFY(db.commit());
        QCOMPARE(db.count(), 2);
    }
    void twoProgramsSeeEachOther() {
        const QString path = dbPath("d.db");
        SongDatabase first(path), second(path);                                // two connections, like two programs
        QVERIFY(!first.changedByOthers());
        QVERIFY(!second.changedByOthers());
        first.insert(row("van de eerste"));
        QVERIFY(second.changedByOthers());                                     // the other one notices it
        QVERIFY(!second.changedByOthers());                                    // ... once
        QVERIFY(!first.changedByOthers());                                     // your own changes are not "others"
        QCOMPARE(second.count(), 1);
        second.insert(row("van de tweede"));
        QVERIFY(first.changedByOthers());
        QCOMPARE(first.count(), 2);
        // only one of them gets to do the first-time work
        QVERIFY(first.claimMeta("klus"));
        QVERIFY(!second.claimMeta("klus"));
        QCOMPARE(second.meta("klus"), QStringLiteral("1"));
        QVERIFY(second.meta("onbekend").isEmpty());
    }
    void makesBackups() {
        SongDatabase db(dbPath("e.db"));
        db.insert(row("in de kopie"));
        const QString dir = tmp.filePath("backups-e");
        db.backupIfDue(dir);
        const QStringList files = QDir(dir).entryList({"songs-*.db"});
        QCOMPARE(files.size(), 1);
        SongDatabase copy(dir + "/" + files[0]);                               // the copy is a database with the song in it
        QCOMPARE(copy.count(), 1);
        db.backupIfDue(dir);                                                    // not again on the same day
        QCOMPARE(QDir(dir).entryList({"songs-*.db"}).size(), 1);
        for (int i = 1; i <= 5; ++i) QFile(dir + QString("/songs-2020-01-0%1.db").arg(i)).open(QIODevice::WriteOnly);
        db.backupIfDue(dir, 3);                                                 // only the newest 3 stay
        const QStringList left = QDir(dir).entryList({"songs-*.db"}, QDir::Files, QDir::Name);
        QCOMPARE(left.size(), 3);
        QVERIFY(left.contains(files[0]));
        QVERIFY(!db.backupTo(dir + "/" + files[0]));                            // never over an existing copy
    }
    void anUnusableFileDoesNotCrash() {
        SongDatabase db(blockedPath());
        QVERIFY(!db.isOpen());
        QVERIFY(!db.error().isEmpty());
        QVERIFY(db.all().isEmpty());
        QCOMPARE(db.insert(row("x")), qint64(0));
        QVERIFY(!db.changedByOthers());
    }

    // ---- the list of songs on top of it --------------------------------------------------
    void libraryFirstRunAndMigration() {
        // nothing yet: one example song
        { SongLibrary lib(dbPath("f.db")); QCOMPARE(lib.rowCount(), 1); QVERIFY(lib.get(0)["title"].toString().contains("Psalm 23")); }
        // the older version kept its songs in songs.json: they are taken over once, and the file is kept as a copy
        const QString dir = tmp.filePath("oud");
        QDir().mkpath(dir);
        QFile f(dir + "/songs.json");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(QJsonArray{
            QJsonObject{{"title", "Oud lied"}, {"number", "3"}, {"lyrics", "[Refrein]\nregel"}, {"author", "Piet"}, {"ccli", "55"},
                        {"order", "Refrein"}, {"category", "Liedboek"}, {"tags", QJsonArray{"Kerst", "Pasen"}}},
            QJsonObject{{"title", "Nog een"}, {"lyrics", "tekst"}}}).toJson());
        f.close();
        {
            SongLibrary lib(dir + "/songs.db");
            QCOMPARE(lib.rowCount(), 2);
            QCOMPARE(lib.get(0)["title"].toString(), QStringLiteral("Oud lied"));
            QCOMPARE(lib.get(0)["tags"].toStringList(), (QStringList{"Kerst", "Pasen"}));
            QCOMPARE(lib.get(0)["ccli"].toString(), QStringLiteral("55"));
            QCOMPARE(lib.get(0)["category"].toString(), QStringLiteral("Liedboek"));
        }
        QVERIFY(!QFile::exists(dir + "/songs.json"));
        QVERIFY(QFile::exists(dir + "/songs.json.migrated"));
        SongLibrary again(dir + "/songs.db");                                    // not taken over a second time
        QCOMPARE(again.rowCount(), 2);
        QVERIFY(QDir(dir + "/backups").entryList({"songs-*.db"}).size() >= 1);   // and a copy of the day was made
    }
    void libraryFollowsTheOtherProgram() {
        const QString path = dbPath("g.db");
        SongLibrary a(path), b(path);
        const int base = a.rowCount();
        QSignalSpy changed(&b, &SongLibrary::externallyChanged);
        const qint64 id = a.add("Van A", "", "tekst", "", "Opwekking", {"Kerst"});
        QVERIFY(id > 0);
        QCOMPARE(b.rowCount(), base);                                            // not yet noticed
        b.checkNow();
        QCOMPARE(b.rowCount(), base + 1);
        QCOMPARE(changed.count(), 1);
        QVERIFY(b.categories().contains("Opwekking"));
        QVERIFY(b.tags().contains("Kerst"));
        b.checkNow();
        QCOMPARE(changed.count(), 1);                                            // nothing new: no signal
        const int row = b.rowOfId(id);
        QVERIFY(row >= 0);
        QCOMPARE(b.get(row)["title"].toString(), QStringLiteral("Van A"));
        // A changes the song, B follows
        QVERIFY(a.update(a.rowOfId(id), "Van A (nieuw)", "", "tekst", "", "Opwekking", {"Kerst"}));
        b.checkNow();
        QCOMPARE(b.get(b.rowOfId(id))["title"].toString(), QStringLiteral("Van A (nieuw)"));
        // A removes it, B notices: the song is gone
        a.remove(a.rowOfId(id));
        b.checkNow();
        QCOMPARE(b.rowOfId(id), -1);
        QCOMPARE(b.rowCount(), base);
        QVERIFY(!b.categories().contains("Opwekking"));
    }
    void libraryDoesNotOverwriteAnotherChange() {
        const QString path = dbPath("h.db");
        SongLibrary a(path), b(path);
        const qint64 id = a.add("Lied", "", "tekst");
        b.checkNow();
        const qint64 rev = b.get(b.rowOfId(id))["rev"].toLongLong();           // B opens the song
        QVERIFY(a.update(a.rowOfId(id), "Lied door A", "", "tekst van A"));    // A saves in the meantime
        b.checkNow();                                                            // B's list is refreshed, but B's editor is still on the old revision
        QVERIFY(!b.update(b.rowOfId(id), "Lied door B", "", "tekst van B", "", "", {}, rev));   // so B's save is refused
        QCOMPARE(b.get(b.rowOfId(id))["title"].toString(), QStringLiteral("Lied door A"));
        const qint64 now = b.get(b.rowOfId(id))["rev"].toLongLong();
        QVERIFY(b.update(b.rowOfId(id), "Lied door B", "", "tekst van B", "", "", {}, now));   // with the current revision it works
        a.checkNow();
        QCOMPARE(a.get(a.rowOfId(id))["title"].toString(), QStringLiteral("Lied door B"));
        // "save anyway" (-1) wins over any revision
        QVERIFY(a.update(a.rowOfId(id), "Nog een", "", "x", "", "", {}, -1));
        // a song that was removed elsewhere cannot be saved
        b.checkNow();
        const int rowB = b.rowOfId(id);
        a.remove(a.rowOfId(id));
        QVERIFY(!b.update(rowB, "Spook", "", "x", "", "", {}, -1));
        QCOMPARE(b.rowOfId(id), -1);
    }
    void libraryKeepsWorkingWithoutAFile() {
        SongLibrary lib(blockedPath());
        QVERIFY(!lib.storageError().isEmpty());
        QVERIFY(lib.rowCount() >= 1);                                            // it works in memory ...
        QVERIFY(lib.add("Tijdelijk", "", "x") > 0);
        QVERIFY(!lib.storageError().isEmpty());                                  // ... and says that nothing is saved
    }
    void severalChangesAtOnce() {
        const QString path = dbPath("i.db");
        {
            SongLibrary lib(path);
            lib.add("Een", "", "x"); lib.add("Twee", "", "x");
            lib.setCategory({0, 1, 2}, "Hillsong");
            lib.addTag({0, 1}, "Kerst");
            lib.addTag({1}, "kerst");
        }
        SongLibrary again(path);
        QCOMPARE(again.rowCount(), 3);
        QCOMPARE(again.get(0)["category"].toString(), QStringLiteral("Hillsong"));
        QCOMPARE(again.get(1)["tags"].toStringList(), (QStringList{"Kerst"}));
        QCOMPARE(again.categories(), (QStringList{"Hillsong"}));
    }
};
QTEST_MAIN(TstSongDatabase)
#include "tst_songdatabase.moc"
