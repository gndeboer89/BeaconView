#include <QtTest>
#include <QDir>
#include <QImage>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include "branding.h"

class TstBranding : public QObject {
    Q_OBJECT
    QTemporaryDir tmp;
    QString png, text;
private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("BeaconViewTest");
        QCoreApplication::setApplicationName("tst_branding");
        QStandardPaths::setTestModeEnabled(true);
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).removeRecursively();
        QSettings().clear();
        QVERIFY(tmp.isValid());
        png = tmp.filePath("church.png");
        QImage img(64, 64, QImage::Format_ARGB32);
        img.fill(Qt::yellow);
        QVERIFY(img.save(png));
        text = tmp.filePath("notes.txt");
        QFile f(text); QVERIFY(f.open(QIODevice::WriteOnly)); f.write("not an image"); f.close();
    }
    void rejectsNonImages() {
        Branding b;
        QVERIFY(!b.chooseLogo(QUrl::fromLocalFile(text)));
        QVERIFY(!b.chooseLogo(QUrl::fromLocalFile(tmp.filePath("missing.png"))));
        QVERIFY(b.source().isEmpty());
    }
    void chooseReplaceAndClear() {
        Branding b;
        QVERIFY(b.chooseLogo(QUrl::fromLocalFile(png)));
        const QString first = QUrl(b.source()).toLocalFile();
        QVERIFY(QFileInfo::exists(first));
        QVERIFY(first.contains("/branding/logo-"));
        QTest::qWait(5);
        QVERIFY(b.chooseLogo(QUrl::fromLocalFile(png)));
        QVERIFY(QUrl(b.source()).toLocalFile() != first);   // new name, so QML does not show a cached image
        QVERIFY(!QFileInfo::exists(first));                  // old copy removed

        Branding again;                                      // persisted
        QCOMPARE(again.source(), b.source());

        b.clearLogo();
        QVERIFY(b.source().isEmpty());
        QVERIFY(!QFileInfo::exists(QUrl(again.source()).toLocalFile()));
        Branding cleared;
        QVERIFY(cleared.source().isEmpty());
    }
    void settingsAreClampedAndPersisted() {
        Branding b;
        b.setScale(5);          QCOMPARE(b.scale(), 1.0);
        b.setScale(0.01);       QCOMPARE(b.scale(), 0.1);
        b.setCorner(9);         QCOMPARE(b.corner(), 3);
        b.setWatermarkSize(1);  QCOMPARE(b.watermarkSize(), 0.4);
        b.setWatermark(true); b.setBgColor("#112233"); b.setScale(0.5);
        Branding again;
        QVERIFY(again.watermark());
        QCOMPARE(again.bgColor(), QStringLiteral("#112233"));
        QCOMPARE(again.scale(), 0.5);
    }
};
QTEST_GUILESS_MAIN(TstBranding)
#include "tst_branding.moc"
