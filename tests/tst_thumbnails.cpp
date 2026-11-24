#include <QtTest>
#include <QGuiApplication>
#include <QImage>
#include <QTemporaryDir>
#include "videothumbnails.h"

class TstThumbnails : public QObject {
    Q_OBJECT
    static QString clip() { return QUrl::fromLocalFile(QStringLiteral(TEST_DATA_DIR "/clip.mp4")).toString(); }
private slots:
    void makesCachesAndForgets() {
        QTemporaryDir cache;
        VideoThumbnails t(cache.path());
        QSignalSpy changed(&t, &VideoThumbnails::changed);

        QCOMPARE(t.thumbnail(clip()), QString());              // first ask: nothing yet, generation starts
        if (!changed.wait(20000)) QSKIP("This computer cannot decode the test video (no usable media backend)");
        const QString url = t.thumbnail(clip());
        QVERIFY(!url.isEmpty());
        QCOMPARE(t.revision(), 1);

        const QImage img(QUrl(url).toLocalFile());
        QVERIFY(!img.isNull());
        QVERIFY(img.width() <= 480 && img.height() <= 270);
        QVERIFY(img.width() == 480 || img.height() == 270);    // scaled to fit, shape kept
        bool colourful = false;                                // a real picture, not an empty frame
        for (int y = 0; y < img.height() && !colourful; y += 7)
            for (int x = 0; x < img.width(); x += 7)
                if (img.pixelColor(x, y).lightness() > 40) { colourful = true; break; }
        QVERIFY(colourful);

        QCOMPARE(t.thumbnail(clip()), url);                    // cached: the same file, no new work
        QCOMPARE(changed.count(), 1);
        t.forget(clip());
        QVERIFY(!QFileInfo::exists(QUrl(url).toLocalFile()));
    }
    void unreadableVideosSimplyHaveNoThumbnail() {
        QTemporaryDir cache, src;
        QFile f(src.filePath("kapot.mp4")); QVERIFY(f.open(QIODevice::WriteOnly)); f.write("geen video"); f.close();
        VideoThumbnails t(cache.path());
        QSignalSpy changed(&t, &VideoThumbnails::changed);
        QCOMPARE(t.thumbnail(QUrl::fromLocalFile(src.filePath("kapot.mp4")).toString()), QString());
        QCOMPARE(t.thumbnail("file:///does/not/exist.mp4"), QString());
        QCOMPARE(t.thumbnail(QString()), QString());
        QTest::qWait(3000);                                    // it fails quietly, no crash, no signal
        QCOMPARE(changed.count(), 0);
        QCOMPARE(t.thumbnail(QUrl::fromLocalFile(src.filePath("kapot.mp4")).toString()), QString());
    }
};
// A GUI application object is needed: video frames are converted to images through the GUI stack.
int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", qgetenv("QT_QPA_PLATFORM").isEmpty() ? "offscreen" : qgetenv("QT_QPA_PLATFORM"));
    QGuiApplication app(argc, argv);
    TstThumbnails test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_thumbnails.moc"
