#include <QtTest>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include "presenter.h"

class TstPresenter : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("BeaconViewTest");
        QCoreApplication::setApplicationName("tst_presenter");
        QStandardPaths::setTestModeEnabled(true);
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).removeRecursively();
        QSettings().clear();
    }
    void slideListAndJumping() {
        Presenter p;
        p.service()->fromJson(QJsonArray());     // start from an empty service
        p.addItem("song", "Lied", "Test", "een\n\ntwee\n\ndrie");
        p.addItem("prayer", "Gebed", "", "");      // no slides: the title is the single slide
        p.selectItem(0);

        QCOMPARE(p.currentSlides(), (QStringList{"een", "twee", "drie"}));
        QCOMPARE(p.currentTitle(), QStringLiteral("Lied – Test"));
        QCOMPARE(p.currentSlide(), 0);

        QSignalSpy live(&p, &Presenter::liveChanged), list(&p, &Presenter::slidesChanged);
        p.goToSlide(2);
        QCOMPARE(p.currentSlide(), 2);
        QCOMPARE(p.liveText(), QStringLiteral("drie"));
        QCOMPARE(live.count(), 1);
        QCOMPARE(list.count(), 0);                 // same part: the list must not be rebuilt (keeps scroll position)

        p.goToSlide(2); QCOMPARE(live.count(), 1); // already there
        p.goToSlide(3); p.goToSlide(-1);           // out of range
        QCOMPARE(p.currentSlide(), 2);

        p.nextSlide();                             // crosses into the next part
        QCOMPARE(p.currentItem(), 1);
        QCOMPARE(p.currentSlides(), (QStringList{"Gebed"}));
        QCOMPARE(list.count(), 1);
        p.prevSlide();                             // and back to the last slide of the previous part
        QCOMPARE(p.currentItem(), 0);
        QCOMPARE(p.currentSlide(), 2);
        QCOMPARE(list.count(), 2);

        p.selectItem(0);                           // same part again: slide resets, list stays
        QCOMPARE(p.currentSlide(), 0);
        QCOMPARE(list.count(), 2);
    }
    void editingRefreshesTheList() {
        Presenter p;
        p.service()->fromJson(QJsonArray());
        p.addItem("song", "Lied", "", "a\n\nb");
        p.selectItem(0);
        QSignalSpy list(&p, &Presenter::slidesChanged);
        p.editItem(0, "Lied", "", "a\n\nb\n\nc");
        QVERIFY(list.count() >= 1);
        QCOMPARE(p.currentSlides().size(), 3);
    }
    void screenEditorApi() {
        Presenter p;
        p.service()->fromJson(QJsonArray());
        const int a = p.addItem("custom", "Eigen", "sub", "");
        QCOMPARE(a, 0);
        QCOMPARE(p.addItem("prayer", "Gebed", "", "Gebed"), 1);
        QVariantMap info = p.itemInfo(a);
        QCOMPARE(info["kind"].toString(), QStringLiteral("custom"));
        QCOMPARE(info["slides"].toStringList().size(), 0);
        QVERIFY(p.itemInfo(9).isEmpty());

        p.editItemSlides(a, "Nieuw", "x", {"een", "", "drie"}, false);
        info = p.itemInfo(a);
        QCOMPARE(info["title"].toString(), QStringLiteral("Nieuw"));
        QCOMPARE(info["slides"].toStringList(), (QStringList{"een", "", "drie"}));   // multi-line text and blank lines survive

        // bible references are only kept when asked for AND the slide count is unchanged
        p.service()->fromJson(QJsonArray());
        QCOMPARE(p.addPassage("Psalm 23:1-4", 2, true, false), 0);
        p.editItemSlides(0, "Schriftlezing", "Psalm 23:1-4", {"x", "y"}, true);
        QCOMPARE(p.itemInfo(0)["refs"].toStringList(), (QStringList{"Psalm 23:1-2", "Psalm 23:3-4"}));
        p.editItemSlides(0, "Schriftlezing", "Psalm 23:1-4", {"x", "y"}, false);
        QVERIFY(p.itemInfo(0)["refs"].toStringList().isEmpty());
        p.addPassage("Psalm 23:1-4", 2, true, false);
        p.editItemSlides(1, "Schriftlezing", "", {"x", "y", "z"}, true);               // count changed: refs would be wrong
        QVERIFY(p.itemInfo(1)["refs"].toStringList().isEmpty());
    }
    void bibleSlidesCarryReferences() {
        Presenter p;
        p.service()->fromJson(QJsonArray());
        QCOMPARE(p.addPassage("Psalm 23:1-4", 2, true, true), 0);
        QCOMPARE(p.currentSlides().size(), 2);
        QCOMPARE(p.currentRefs(), (QStringList{"Psalm 23:1-2", "Psalm 23:3-4"}));
        QCOMPARE(p.addPassage("Nonsens 99", 2, true, true), -1);
    }
};
QTEST_GUILESS_MAIN(TstPresenter)
#include "tst_presenter.moc"
