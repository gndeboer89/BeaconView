#include <QtTest>
#include <QDir>
#include <QImage>
#include <QSettings>
#include <QTemporaryDir>
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

        p.editItemSlides(a, "Nieuw", "x", {"een", "", "drie"}, {}, 0, false);
        info = p.itemInfo(a);
        QCOMPARE(info["title"].toString(), QStringLiteral("Nieuw"));
        QCOMPARE(info["slides"].toStringList(), (QStringList{"een", "", "drie"}));   // multi-line text and blank lines survive

        // bible references are only kept when asked for AND the slide count is unchanged
        p.service()->fromJson(QJsonArray());
        QCOMPARE(p.addPassage("Psalm 23:1-4", 2, true, false), 0);
        p.editItemSlides(0, "Schriftlezing", "Psalm 23:1-4", {"x", "y"}, {}, 0, true);
        QCOMPARE(p.itemInfo(0)["refs"].toStringList(), (QStringList{"Psalm 23:1-2", "Psalm 23:3-4"}));
        p.editItemSlides(0, "Schriftlezing", "Psalm 23:1-4", {"x", "y"}, {}, 0, false);
        QVERIFY(p.itemInfo(0)["refs"].toStringList().isEmpty());
        p.addPassage("Psalm 23:1-4", 2, true, false);
        p.editItemSlides(1, "Schriftlezing", "", {"x", "y", "z"}, {}, 0, true);               // count changed: refs would be wrong
        QVERIFY(p.itemInfo(1)["refs"].toStringList().isEmpty());
    }
    void hideTextKeepsTheBackground() {
        Presenter p;
        p.service()->fromJson(QJsonArray());
        p.addItem("prayer", "Gebed", "", "een\n\ntwee");
        p.addItem("prayer", "Zegen", "", "zegen");
        p.selectItem(0);
        QSignalSpy spy(&p, &Presenter::textHiddenChanged);
        QVERIFY(!p.textHidden());
        p.setTextHidden(true);
        QVERIFY(p.textHidden());
        QCOMPARE(p.liveText(), QStringLiteral("een"));           // the slide itself is untouched, only its display is hidden
        p.setTextHidden(true);
        QCOMPARE(spy.count(), 1);                                // no duplicate signal

        p.nextSlide();                                           // every kind of slide change shows the text again
        QVERIFY(!p.textHidden());
        p.setTextHidden(true); p.goToSlide(0); QVERIFY(!p.textHidden());
        p.setTextHidden(true); p.goToSlide(0); QVERIFY(!p.textHidden());   // clicking the slide that is already live brings the text back
        p.setTextHidden(true); p.prevSlide();  QVERIFY(p.textHidden());    // already on the very first slide: nothing changes, so the text stays hidden
        p.setTextHidden(true); p.selectItem(1); QVERIFY(!p.textHidden());
        p.setTextHidden(true); p.prevSlide();   QVERIFY(!p.textHidden());  // back into the previous part
    }
    void songKeepsItsPartNames() {
        Presenter p;
        p.service()->fromJson(QJsonArray());
        p.songs()->add("Genade", "", "[Couplet 1]\nregel een\n\n[Refrein]\nrefrein");
        p.addSong(p.songs()->rowCount() - 1);
        p.selectItem(0);
        QCOMPARE(p.currentSlides(), (QStringList{"regel een", "refrein"}));      // the output only gets the text
        QCOMPARE(p.currentLabels(), (QStringList{"Couplet 1", "Refrein"}));      // the operator and the editor see the names
        QCOMPARE(p.liveText(), QStringLiteral("regel een"));
        QCOMPARE(p.itemText(0), QStringLiteral("[Couplet 1]\nregel een\n\n[Refrein]\nrefrein"));
        p.editItem(0, "Lied", "Genade", "[Vers A]\nnieuw\n\nzonder naam");     // after editing, the names follow the text
        QCOMPARE(p.currentSlides(), (QStringList{"nieuw", "zonder naam"}));
        QCOMPARE(p.currentLabels(), (QStringList{"Vers A", ""}));
        p.editItemSlides(0, "Lied", "", {"x"}, {}, 0, false);                 // the screen editor does not know names
        QCOMPARE(p.currentLabels(), (QStringList{""}));
    }
    void songListIsSortedAndSearchable() {
        Presenter p;
        p.songs()->add("Zondagslied", "", "tekst");
        p.songs()->add("Aanbidding", "", "andere tekst");
        QCOMPARE(p.songFilter()->total(), p.songs()->rowCount());          // the list of the app is the library
        QCOMPARE(p.songFilter()->rowCount(), p.songs()->rowCount());
        p.songFilter()->setQuery("aanbidding");
        QCOMPARE(p.songFilter()->rowCount(), 1);
        const int src = p.songFilter()->sourceRow(0);
        p.addSong(src);                                                    // "Aan dienst" uses the row in the library
        QCOMPARE(p.service()->item(p.service()->count() - 1)->subtitle, QStringLiteral("Aanbidding"));
    }
    void songOrderStandardAndOwn() {
        Presenter p;
        p.service()->fromJson(QJsonArray());
        p.songs()->add("Genade", "", "[Couplet 1]\nregel 1\n\n[Refrein]\nrefrein\n\n[Couplet 2]\nregel 2",
                       "Couplet 1, Refrein, Couplet 2, Refrein");
        const int song = p.songs()->rowCount() - 1;
        p.addSong(song);
        p.selectItem(0);
        // a new part uses the standard order of the song
        QCOMPARE(p.currentSlides(), (QStringList{"regel 1", "refrein", "regel 2", "refrein"}));
        QCOMPARE(p.currentLabels(), (QStringList{"Couplet 1", "Refrein", "Couplet 2", "Refrein"}));
        QCOMPARE(p.itemOrder(0), QStringLiteral("Couplet 1, Refrein, Couplet 2, Refrein"));
        QCOMPARE(p.itemDefaultOrder(0), p.itemOrder(0));
        QCOMPARE(p.itemText(0), QStringLiteral("[Couplet 1]\nregel 1\n\n[Refrein]\nrefrein\n\n[Couplet 2]\nregel 2"));   // the editor gets every part once
        // an own order for this service only
        p.editItem(0, "Lied", "Genade", p.itemText(0), "Refrein, Couplet 1, Refrein");
        QCOMPARE(p.currentSlides(), (QStringList{"refrein", "regel 1", "refrein"}));
        QCOMPARE(p.itemDefaultOrder(0), QStringLiteral("Couplet 1, Refrein, Couplet 2, Refrein"));   // the standard order is kept
        QCOMPARE(p.songs()->data(p.songs()->index(song), SongLibrary::OrderRole).toString(), QStringLiteral("Couplet 1, Refrein, Couplet 2, Refrein"));
        // back to the standard order, and a song without an order
        p.editItem(0, "Lied", "Genade", p.itemText(0), p.itemDefaultOrder(0));
        QCOMPARE(p.currentSlides().size(), 4);
        p.editItem(0, "Lied", "Genade", p.itemText(0), "");
        QCOMPARE(p.currentSlides(), (QStringList{"regel 1", "refrein", "regel 2"}));
        // an own order survives saving and loading the service
        p.editItem(0, "Lied", "Genade", p.itemText(0), "Refrein, Refrein");
        Presenter q;
        q.service()->fromJson(p.service()->toJson());
        QCOMPARE(q.service()->item(0)->slides, (QStringList{"refrein", "refrein"}));
        QCOMPARE(q.service()->item(0)->order, (QStringList{"Refrein", "Refrein"}));
        // services from before the order existed still work
        Presenter r;
        r.service()->fromJson(QJsonArray{QJsonObject{{"kind", "song"}, {"title", "Lied"}, {"subtitle", "Oud"}, {"slides", QJsonArray{"a", "b"}}}});
        QCOMPARE(r.itemText(0), QStringLiteral("a\n\nb"));
        r.editItem(0, "Lied", "Oud", "a\n\nb\n\nc");
        QCOMPARE(r.service()->item(0)->slides.size(), 3);
    }
    void usageIsRecordedWhenShownLive() {
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).removeRecursively();
        Presenter p;
        p.service()->fromJson(QJsonArray());
        p.songs()->add("Eerste lied", "", "tekst een");
        p.songs()->add("Tweede lied", "", "tekst twee");
        p.addSong(p.songs()->rowCount() - 2);
        p.addSong(p.songs()->rowCount() - 1);
        p.addItem("prayer", "Gebed", "", "bidden");
        const int before = p.usage()->count();
        p.selectItem(0);                                         // output closed: rehearsing does not count
        p.selectItem(1);
        QCOMPARE(p.usage()->count(), before);
        p.setOutputOn(true);                                     // opening the output: the live song counts
        QCOMPARE(p.usage()->count(), before + 1);
        p.selectItem(0);                                         // the first song comes live
        p.selectItem(0);                                         // ...selecting it again is not a second time
        p.nextSlide();
        QCOMPARE(p.usage()->count(), before + 2);
        p.selectItem(1);                                         // shown again in this session: still once
        QCOMPARE(p.usage()->count(), before + 2);
        p.selectItem(2);                                         // a prayer is no song
        QCOMPARE(p.usage()->count(), before + 2);
        p.setOutputOn(false);
        p.setOutputOn(true);                                     // the next service: they count again
        p.selectItem(0);
        QCOMPARE(p.usage()->count(), before + 3);
        QCOMPARE(p.usage()->uses("all") >= 3, true);
    }
    void dragMoveKeepsLiveItem() {
        Presenter p;
        p.service()->fromJson(QJsonArray());                      // start without the demo service
        p.addItem("prayer", "A", "", "a");
        p.addItem("prayer", "B", "", "b");
        p.addItem("prayer", "C", "", "c");
        p.addItem("prayer", "D", "", "d");
        p.selectItem(1);                                          // B is live
        p.moveItemTo(0, 3);                                       // A to the end: B moves up, but stays live
        QCOMPARE(p.currentItem(), 0);
        QCOMPARE(p.liveText(), QStringLiteral("b"));
        p.moveItemTo(0, 2);                                       // the live part itself is dragged
        QCOMPARE(p.currentItem(), 2);
        QCOMPARE(p.liveText(), QStringLiteral("b"));
        p.moveItemTo(3, 0);                                       // something from below is dropped above the live part
        QCOMPARE(p.currentItem(), 3);
        QCOMPARE(p.liveText(), QStringLiteral("b"));
        p.moveItem(3, -1);                                        // the menu items use the same code
        QCOMPARE(p.currentItem(), 2);
        QCOMPARE(p.liveText(), QStringLiteral("b"));
        p.moveItemTo(2, 2); p.moveItemTo(2, 9);                   // nothing to do / out of range
        QCOMPARE(p.currentItem(), 2);
    }
    void blankSlideIsKept() {
        Presenter p;
        p.service()->fromJson(QJsonArray());
        const int a = p.addItem("custom", "Stilte", "", "");
        p.editItemSlides(a, "Stilte", "", {""}, {}, 0, false);     // one explicitly empty slide
        p.selectItem(a);
        QCOMPARE(p.slideCount(), 1);
        QCOMPARE(p.liveText(), QString());                         // blank: the title must NOT show up instead
        p.editItemSlides(a, "Stilte", "", {"Tekst", "", "Tekst 2"}, {}, 0, false);
        QCOMPARE(p.itemInfo(a)["slides"].toStringList(), (QStringList{"Tekst", "", "Tekst 2"}));
        p.goToSlide(1);
        QCOMPARE(p.liveText(), QString());
    }
    void upcomingVideoIsPreloadable() {
        Presenter p;
        p.service()->fromJson(QJsonArray());
        p.addItem("prayer", "Gebed", "", "Gebed");
        const int v = p.addVideo(QUrl::fromLocalFile("/tmp/film.mp4"));
        p.addItem("blessing", "Zegen", "", "Zegen");
        const int v2 = p.addVideo(QUrl::fromLocalFile("/tmp/film2.mp4"));
        p.selectItem(0);
        QCOMPARE(p.upcomingVideo(), QStringLiteral("file:///tmp/film.mp4"));   // the next part is a video: load it in advance
        p.selectItem(v);
        QCOMPARE(p.upcomingVideo(), QString());                                // playing a video: nothing is preloaded
        p.selectItem(v + 1);
        QCOMPARE(p.upcomingVideo(), QStringLiteral("file:///tmp/film2.mp4"));
        p.selectItem(v2);
        QCOMPARE(p.upcomingVideo(), QString());
    }
    void logFileStaysSmall() {
        Presenter p;
        QFile::remove(p.logPath());
        p.log("eerste regel");
        QFile f(p.logPath());
        QVERIFY(f.open(QIODevice::ReadOnly));
        QVERIFY(QString::fromUtf8(f.readAll()).contains("eerste regel"));
        f.close();
        const QString filler(200, 'x');
        for (int i = 0; i < 2000; ++i) p.log(filler);                           // ~400 KB: must rotate
        QVERIFY(QFileInfo(p.logPath()).size() < 300 * 1024);
        QVERIFY(QFileInfo(QFileInfo(p.logPath()).absolutePath() + "/log.old.txt").exists());
    }
    void removeMediaCleansUp() {
        Presenter p;
        p.service()->fromJson(QJsonArray());
        QTemporaryDir src;
        QImage img(20, 20, QImage::Format_RGB32); img.fill(Qt::blue);
        QVERIFY(img.save(src.filePath("plaatje.png")));
        p.importMedia(QUrl::fromLocalFile(src.filePath("plaatje.png")));
        const QString url = p.mediaFiles().last();
        const QString copy = QUrl(url).toLocalFile();
        QVERIFY(QFileInfo::exists(copy));
        QVERIFY(copy.contains("/media/"));

        const int a = p.addItem("prayer", "Gebed", "", "Gebed");
        p.addItem("prayer", "Zegen", "", "Zegen");
        p.setItemBackground(a, url);
        p.setDefaultBackground(url);
        const auto use = p.mediaUsage(url);
        QCOMPARE(use["parts"].toInt(), 1);
        QVERIFY(use["isDefault"].toBool());
        QCOMPARE(p.mediaInfo(url)["kind"].toString(), QStringLiteral("image"));
        QVERIFY(p.mediaInfo(url)["exists"].toBool());
        QCOMPARE(p.mediaInfo(url)["name"].toString(), QFileInfo(copy).fileName());

        QSignalSpy changed(&p, &Presenter::mediaFilesChanged);
        p.removeMedia(url);
        QCOMPARE(changed.count(), 1);
        QVERIFY(!p.mediaFiles().contains(url));
        QVERIFY(!QFileInfo::exists(copy));                          // our copy is deleted
        QVERIFY(QFileInfo::exists(src.filePath("plaatje.png")));    // the original is never touched
        QCOMPARE(p.itemInfo(a)["background"].toString(), QString()); // part falls back to the default
        QCOMPARE(p.defaultBackground(), QStringLiteral("g:sunset"));
        p.removeMedia(url);                                          // unknown now: nothing happens
        QCOMPARE(changed.count(), 1);
    }
    void videoPartCountsAsUsage() {
        Presenter p;
        p.service()->fromJson(QJsonArray());
        p.addVideo(QUrl("file:///media/film.mp4"));
        QCOMPARE(p.mediaUsage("file:///media/film.mp4")["parts"].toInt(), 1);
        QCOMPARE(p.mediaInfo("file:///media/film.mp4")["kind"].toString(), QStringLiteral("video"));
        QVERIFY(!p.mediaInfo("file:///media/film.mp4")["exists"].toBool());
    }
    void imageSlides() {
        Presenter p;
        p.service()->fromJson(QJsonArray());
        const int a = p.addSlidesItem("slides", "Preek", {"file:///a.png", "file:///b.png", "file:///c.png"}, 0);
        QCOMPARE(a, 0);
        p.selectItem(a);
        QCOMPARE(p.slideCount(), 3);
        QCOMPARE(p.currentKind(), QStringLiteral("slides"));
        QCOMPARE(p.liveImage(), QStringLiteral("file:///a.png"));
        QCOMPARE(p.liveText(), QString());                       // an image slide shows no text (and no title fallback)
        p.nextSlide();
        QCOMPARE(p.liveImage(), QStringLiteral("file:///b.png"));
        QCOMPARE(p.currentImages(), (QStringList{"file:///a.png", "file:///b.png", "file:///c.png"}));

        // a text slide between image slides
        p.editItemSlides(a, "Preek", "", {"", "Tekst", ""}, {"file:///a.png", "", "file:///c.png"}, 0, false);
        QCOMPARE(p.itemInfo(a)["images"].toStringList(), (QStringList{"file:///a.png", "", "file:///c.png"}));
        // lists stay the same length even when the caller sends fewer images
        p.editItemSlides(a, "Preek", "", {"x", "y", "z", "w"}, {"file:///a.png"}, 0, false);
        QCOMPARE(p.itemInfo(a)["images"].toStringList().size(), 4);
    }
    void announcementLoop() {
        Presenter p;
        p.service()->fromJson(QJsonArray());
        const int loop = p.addSlidesItem("loop", "Mededelingen", {"file:///1.png", "file:///2.png", "file:///3.png"}, 10);
        p.addItem("prayer", "Gebed", "", "Gebed");
        p.selectItem(loop);
        QCOMPARE(p.currentLoopSeconds(), 10);
        QSignalSpy list(&p, &Presenter::slidesChanged);

        p.loopTick(); QCOMPARE(p.currentSlide(), 1);
        p.loopTick(); QCOMPARE(p.currentSlide(), 2);
        p.loopTick(); QCOMPARE(p.currentSlide(), 0);              // the automatic loop wraps around
        QCOMPARE(p.currentItem(), 0);
        QCOMPARE(list.count(), 0);                                // ...without rebuilding the slide list

        p.goToSlide(2);
        p.nextSlide();                                            // manual "next" on the last slide leaves the loop
        QCOMPARE(p.currentItem(), 1);
        QCOMPARE(p.currentLoopSeconds(), 0);

        p.setLoopPaused(true);
        QVERIFY(p.loopPaused());
        p.selectItem(loop);
        p.setLoopPaused(false);
        QVERIFY(!p.loopPaused());
    }
    void videoPart() {
        Presenter p;
        p.service()->fromJson(QJsonArray());
        p.addItem("prayer", "Gebed", "", "Gebed");
        const int v = p.addVideo(QUrl::fromLocalFile("/tmp/Opening film.mp4"));
        QCOMPARE(p.itemInfo(v)["kind"].toString(), QStringLiteral("video"));
        QCOMPARE(p.itemInfo(v)["title"].toString(), QStringLiteral("Opening film"));

        QSignalSpy start(&p, &Presenter::videoStart);
        p.selectItem(v);
        QCOMPARE(start.count(), 1);
        QCOMPARE(p.liveVideo(), QStringLiteral("file:///tmp/Opening film.mp4"));
        QCOMPARE(p.liveText(), QString());                        // no title text over the video
        p.selectItem(v);
        QCOMPARE(start.count(), 2);                               // selecting it again restarts it
        QVERIFY(!p.liveVideoLoop() && !p.liveVideoNext());

        p.editVideo(v, "Film", QUrl(), true, true);
        QVERIFY(p.liveVideoLoop() && p.liveVideoNext());
        QCOMPARE(p.itemInfo(v)["title"].toString(), QStringLiteral("Film"));
        QCOMPARE(p.liveVideo(), QStringLiteral("file:///tmp/Opening film.mp4"));   // empty URL keeps the file

        p.selectItem(0);
        QCOMPARE(p.liveVideo(), QString());                       // leaving the video part
        QVERIFY(!p.fileExists("file:///tmp/does-not-exist.mp4"));
        QVERIFY(!p.fileExists(""));
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
