#include <QtTest>
#include <QImage>

// Guards against a broken resource path: the logo and icon must be bundled and readable.
class TstAssets : public QObject {
    Q_OBJECT
private slots:
    void logoIsBundled() {
        QImage img(":/assets/logo.png");
        QVERIFY(!img.isNull());
        QVERIFY(img.hasAlphaChannel());
        QVERIFY(img.width() > img.height());          // wide logo with text
    }
    void iconIsBundled() {
        QImage img(":/assets/icon/icon.png");
        QVERIFY(!img.isNull());
        QCOMPARE(img.width(), img.height());          // square
        QVERIFY(img.width() >= 128);
    }
};
QTEST_GUILESS_MAIN(TstAssets)
#include "tst_assets.moc"
