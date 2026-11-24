#include <QtTest>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryDir>
#include "songimporter.h"
#include "songlibrary.h"

static const char *kTxt =
    "Amazing Grace\n"
    "Verse 1\n"
    "Amazing grace how sweet the sound\n"
    "That saved a wretch like me\n"
    "\n"
    "Chorus\n"
    "My chains are gone\n"
    "I've been set free\n"
    "Verse 2\n"
    "T'was grace that taught\n"
    "my heart to fear\n"
    "\n"
    "CCLI Song # 4768151\n"
    "John Newton | Edwin Excell\n"
    "\xC2\xA9 Public Domain\n"
    "For use solely with the SongSelect Terms of Use. All rights reserved. www.ccli.com\n"
    "CCLI License # 123456\n";

static const char *kUsr =
    "[File]\r\n"
    "Type=SongSelect Import File\r\n"
    "Version=3\r\n"
    "[S A4768151]\r\n"
    "Title=Amazing Grace\r\n"
    "Author=John Newton/Edwin Excell\r\n"
    "Copyright=Public Domain\r\n"
    "Admin=\r\n"
    "Fields=Verse 1/tChorus/tVerse 2\r\n"
    "Words=Amazing grace how sweet the sound/nThat saved a wretch like me/tMy chains are gone/nI've been set free/tT'was grace that taught/nmy heart to fear\r\n";

class TstSongImport : public QObject {
    Q_OBJECT
    QTemporaryDir tmp;
    QString write(const QString &name, const QByteArray &data) {
        const QString p = tmp.filePath(name);
        QFile f(p); f.open(QIODevice::WriteOnly); f.write(data); f.close();
        return p;
    }
private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("BeaconViewTest");
        QCoreApplication::setApplicationName("tst_songimport");
        QStandardPaths::setTestModeEnabled(true);
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).removeRecursively();
        QVERIFY(tmp.isValid());
    }
    void textFile() {
        const auto songs = SongImporter::parse(QByteArray(kTxt), "Amazing Grace.txt");
        QCOMPARE(songs.size(), 1);
        const ImportedSong &s = songs[0];
        QCOMPARE(s.title, QStringLiteral("Amazing Grace"));
        QCOMPARE(s.ccli, QStringLiteral("4768151"));
        QCOMPARE(s.author, QStringLiteral("John Newton, Edwin Excell"));
        QVERIFY(s.copyright.contains("Public Domain"));
        // part labels are gone; each part is a slide (a blank line), also when the file had no blank line before a label
        QCOMPARE(s.lyrics, QStringLiteral("Amazing grace how sweet the sound\nThat saved a wretch like me\n\n"
                                          "My chains are gone\nI've been set free\n\n"
                                          "T'was grace that taught\nmy heart to fear"));
        QVERIFY(!s.lyrics.contains("CCLI"));
        QVERIFY(!s.lyrics.contains("For use solely"));
    }
    void usrFile() {
        const auto songs = SongImporter::parse(QByteArray(kUsr), "song.usr");
        QCOMPARE(songs.size(), 1);
        QCOMPARE(songs[0].title, QStringLiteral("Amazing Grace"));
        QCOMPARE(songs[0].ccli, QStringLiteral("4768151"));
        QCOMPARE(songs[0].author, QStringLiteral("John Newton, Edwin Excell"));
        QCOMPARE(songs[0].lyrics, QStringLiteral("Amazing grace how sweet the sound\nThat saved a wretch like me\n\n"
                                                 "My chains are gone\nI've been set free\n\n"
                                                 "T'was grace that taught\nmy heart to fear"));
        // the content decides, not the extension
        QCOMPARE(SongImporter::parse(QByteArray(kUsr), "song.txt").size(), 1);
    }
    void plainTextWithoutFooter() {
        const auto songs = SongImporter::parse("Mijn lied\r\n\r\nEerste regel\r\ntweede regel\r\n\r\nDerde regel\r\n", "mijn.txt");
        QCOMPARE(songs.size(), 1);
        QCOMPARE(songs[0].title, QStringLiteral("Mijn lied"));
        QCOMPARE(songs[0].ccli, QString());
        QCOMPARE(songs[0].lyrics, QStringLiteral("Eerste regel\ntweede regel\n\nDerde regel"));
    }
    void dutchLabelsAndEncodings() {
        const QString text = QStringLiteral("Genade\nCouplet 1\nGenade, zo groot\n\nRefrein (x2)\nOnze God is goed\n");
        auto songs = SongImporter::parseText(text);
        QCOMPARE(songs[0].lyrics, QStringLiteral("Genade, zo groot\n\nOnze God is goed"));
        // UTF-8 with BOM, UTF-16 with BOM and Latin-1 (Windows) all give the same accents
        const QString accent = QStringLiteral("Vrede\nGenade voor ons één\n");
        QCOMPARE(SongImporter::decode("\xEF\xBB\xBF" + accent.toUtf8()), accent);
        QCOMPARE(SongImporter::decode(accent.toUtf8()), accent);
        QByteArray u16("\xFF\xFE"); u16.append(reinterpret_cast<const char *>(accent.utf16()), accent.size() * 2);
        QCOMPARE(SongImporter::decode(u16), accent);
        QCOMPARE(SongImporter::decode(accent.toLatin1()), accent);
    }
    void rejectsGarbage() {
        QVERIFY(SongImporter::parse("", "a.txt").isEmpty());
        QVERIFY(SongImporter::parse("Alleen een titel\n", "a.txt").isEmpty());
        QVERIFY(SongImporter::parse("[File]\nType=x\n", "a.usr").isEmpty());
    }
    void libraryImport() {
        SongLibrary lib;
        const int before = lib.rowCount();
        const QString a = write("a.txt", kTxt), b = write("b.usr", kUsr), c = write("c.txt", "Los lied\n\nregel een\n"),
                      bad = write("bad.txt", "titel zonder tekst\n");
        QVariantMap r = lib.importFiles({QUrl::fromLocalFile(a), QUrl::fromLocalFile(c), QUrl::fromLocalFile(bad), QUrl::fromLocalFile(tmp.filePath("missing.txt"))});
        QCOMPARE(r["added"].toInt(), 2);
        QCOMPARE(r["failed"].toInt(), 2);
        QCOMPARE(lib.rowCount(), before + 2);
        // the same song again (also as a .usr, found by its CCLI number) is not added twice
        r = lib.importFiles({QUrl::fromLocalFile(a), QUrl::fromLocalFile(b)});
        QCOMPARE(r["added"].toInt(), 0);
        QCOMPARE(r["existing"].toInt(), 2);
        QCOMPARE(lib.rowCount(), before + 2);
        // credits are stored, survive an edit, and survive a restart
        const QModelIndex i = lib.index(before);
        QCOMPARE(lib.data(i, SongLibrary::CcliRole).toString(), QStringLiteral("4768151"));
        lib.update(before, "Amazing Grace (kort)", "", "regel");
        QCOMPARE(lib.data(i, SongLibrary::CcliRole).toString(), QStringLiteral("4768151"));
        QCOMPARE(lib.data(i, SongLibrary::AuthorRole).toString(), QStringLiteral("John Newton, Edwin Excell"));
        SongLibrary again;
        QCOMPARE(again.rowCount(), before + 2);
        QCOMPARE(again.data(again.index(before), SongLibrary::CcliRole).toString(), QStringLiteral("4768151"));
    }
};
QTEST_MAIN(TstSongImport)
#include "tst_songimport.moc"
