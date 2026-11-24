#include <QtTest>
#include <QImage>
#include <QTemporaryDir>
#include "slideimporter.h"

class TstImporter : public QObject {
    Q_OBJECT
    QTemporaryDir out;
    static QUrl data(const char *name) { return QUrl::fromLocalFile(QStringLiteral(TEST_DATA_DIR "/") + name); }
private slots:
    void initTestCase() { QVERIFY(out.isValid()); }

    void pdfPagesBecomeSlides() {
        const auto r = SlideImporter::importSync({data("sample.pdf")}, out.path());
        QVERIFY2(r.ok(), qPrintable(r.error));
        QCOMPARE(r.images.size(), 3);
        QCOMPARE(r.title, QStringLiteral("sample"));

        QList<QSize> sizes;
        for (const QString &u : r.images) {
            const QString f = QUrl(u).toLocalFile();
            QVERIFY(QFileInfo::exists(f));
            const QImage img(f);
            QVERIFY(!img.isNull());
            QVERIFY(img.width() <= 1920 && img.height() <= 1080);
            QVERIFY(img.width() == 1920 || img.height() == 1080);     // scaled up to fill the 1920x1080 frame
            QCOMPARE(img.pixelColor(2, 2), QColor(Qt::white));        // white paper, not transparent
            bool hasInk = false;                                       // the page text was rendered
            for (int y = 0; y < img.height() && !hasInk; y += 3)
                for (int x = 0; x < img.width(); x += 3)
                    if (img.pixelColor(x, y).lightness() < 80) { hasInk = true; break; }
            QVERIFY2(hasInk, "page looks blank");
            sizes << img.size();
        }
        QCOMPARE(sizes[0], QSize(1920, 1080));      // 16:9 page
        QCOMPARE(sizes[1], QSize(1440, 1080));      // 4:3 page keeps its shape
        QCOMPARE(sizes[2], QSize(810, 1080));       // portrait page keeps its shape
    }

    void picturesAreCopied() {
        QTemporaryDir src;
        QImage a(100, 50, QImage::Format_RGB32); a.fill(Qt::red);
        QImage b(60, 60, QImage::Format_RGB32); b.fill(Qt::blue);
        QVERIFY(a.save(src.filePath("een.png"))); QVERIFY(b.save(src.filePath("twee.jpg")));
        const auto r = SlideImporter::importSync({QUrl::fromLocalFile(src.filePath("een.png")), QUrl::fromLocalFile(src.filePath("twee.jpg"))}, out.path());
        QVERIFY2(r.ok(), qPrintable(r.error));
        QCOMPARE(r.images.size(), 2);
        QCOMPARE(r.title, QStringLiteral("een (+1)"));
        QCOMPARE(QImage(QUrl(r.images[0]).toLocalFile()).size(), QSize(100, 50));   // copied as they are
        QVERIFY(QUrl(r.images[1]).toLocalFile().endsWith(".jpg"));
    }

    void mixedFilesKeepTheirOrder() {
        QTemporaryDir src;
        QImage a(10, 10, QImage::Format_RGB32); a.fill(Qt::green);
        QVERIFY(a.save(src.filePath("voor.png")));
        const auto r = SlideImporter::importSync({QUrl::fromLocalFile(src.filePath("voor.png")), data("sample.pdf")}, out.path());
        QVERIFY2(r.ok(), qPrintable(r.error));
        QCOMPARE(r.images.size(), 4);
        QCOMPARE(QImage(QUrl(r.images[0]).toLocalFile()).size(), QSize(10, 10));
        QCOMPARE(QImage(QUrl(r.images[1]).toLocalFile()).size(), QSize(1920, 1080));
    }

    void errorsAreClearAndLeaveNothingBehind() {
        QTemporaryDir fresh;
        QTemporaryDir src;
        QFile f(src.filePath("kapot.pdf")); QVERIFY(f.open(QIODevice::WriteOnly)); f.write("dit is geen pdf"); f.close();
        QFile g(src.filePath("notities.txt")); QVERIFY(g.open(QIODevice::WriteOnly)); g.write("x"); g.close();

        auto bad = SlideImporter::importSync({QUrl::fromLocalFile(src.filePath("kapot.pdf"))}, fresh.path());
        QVERIFY(!bad.ok()); QVERIFY(bad.error.contains("PDF"));
        bad = SlideImporter::importSync({QUrl::fromLocalFile(src.filePath("notities.txt"))}, fresh.path());
        QVERIFY(bad.error.contains("niet ondersteund"));
        bad = SlideImporter::importSync({QUrl::fromLocalFile(src.filePath("weg.png"))}, fresh.path());
        QVERIFY(bad.error.contains("niet gevonden"));
        bad = SlideImporter::importSync({}, fresh.path());
        QVERIFY(!bad.ok());
        QVERIFY2(QDir(fresh.path()).entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty(), "failed imports must not leave folders behind");
    }

    void powerPointViaLibreOffice() {
        if (SlideImporter::findLibreOffice().isEmpty()) QSKIP("LibreOffice is not installed");
        const auto r = SlideImporter::importSync({data("sample.pptx")}, out.path());
        QVERIFY2(r.ok(), qPrintable(r.error));
        QCOMPARE(r.images.size(), 3);
        const QImage first(QUrl(r.images[0]).toLocalFile());
        QCOMPARE(first.size(), QSize(1920, 1080));                    // the deck is 16:9
    }

    void asyncImportReportsBack() {
        SlideImporter imp(out.path());
        QSignalSpy done(&imp, &SlideImporter::finished), fail(&imp, &SlideImporter::failed);
        imp.importFiles("mijn-tag", {data("sample.pdf")});
        QVERIFY(imp.busy());
        QTRY_COMPARE_WITH_TIMEOUT(done.count(), 1, 20000);
        QCOMPARE(fail.count(), 0);
        QCOMPARE(done.first().at(0).toString(), QStringLiteral("mijn-tag"));
        QCOMPARE(done.first().at(1).toStringList().size(), 3);
        QVERIFY(!imp.busy());

        QSignalSpy fail2(&imp, &SlideImporter::failed);
        imp.importFiles("fout", {QUrl::fromLocalFile("/nope/missing.pdf")});
        QTRY_COMPARE_WITH_TIMEOUT(fail2.count(), 1, 20000);
        QCOMPARE(fail2.first().at(0).toString(), QStringLiteral("fout"));
    }
};
QTEST_GUILESS_MAIN(TstImporter)
#include "tst_importer.moc"
