#include <QtTest>
#include "servicemodel.h"

class TstService : public QObject {
    Q_OBJECT
private slots:
    void templateRoundTrip() {
        ServiceModel m;
        m.append({"song", "Lied", "x", {}, {"a", "b"}, {}, {}});
        m.append({"prayer", "Gebed", "", {}, {"Gebed"}, {}, {}});
        QCOMPARE(m.data(m.index(0), ServiceModel::TemplateRole).toString(), QString());

        QSignalSpy spy(&m, &QAbstractItemModel::dataChanged);
        m.setTemplate(1, "builtin:elegant");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(m.data(m.index(1), ServiceModel::TemplateRole).toString(), QStringLiteral("builtin:elegant"));
        m.setTemplate(1, "builtin:elegant");     // unchanged: no signal
        QCOMPARE(spy.count(), 1);
        m.setTemplate(7, "x");                    // out of range: ignored

        ServiceModel copy;
        copy.fromJson(m.toJson());
        QCOMPARE(copy.count(), 2);
        QCOMPARE(copy.item(1)->templateId, QStringLiteral("builtin:elegant"));
        QVERIFY(copy.item(0)->templateId.isEmpty());
    }
    void oldFilesWithoutTemplateStillLoad() {
        QJsonArray arr;
        arr.append(QJsonObject{{"kind", "song"}, {"title", "Oud"}, {"slides", QJsonArray{"a"}}});
        ServiceModel m;
        m.fromJson(arr);
        QCOMPARE(m.count(), 1);
        QVERIFY(m.item(0)->templateId.isEmpty());
    }
    void imagesLoopAndVideoRoundTrip() {
        ServiceModel m;
        ServiceItem it{"loop", "Mededelingen", "", {}, {"", ""}, {}, {}};
        it.images = {"file:///a.png", "file:///b.png"};
        it.loopSeconds = 12;
        ServiceItem v{"video", "Film", "", {}, {}, {}, {}};
        v.video = "file:///film.mp4"; v.videoLoop = true; v.videoNext = true;
        m.append(it); m.append(v);

        ServiceModel copy;
        copy.fromJson(m.toJson());
        QCOMPARE(copy.item(0)->images, (QStringList{"file:///a.png", "file:///b.png"}));
        QCOMPARE(copy.item(0)->loopSeconds, 12);
        QCOMPARE(copy.item(1)->video, QStringLiteral("file:///film.mp4"));
        QVERIFY(copy.item(1)->videoLoop && copy.item(1)->videoNext);
    }
    void slideNamesAreSplitAndJoined() {
        QStringList labels;
        const QString text = "[Verse 1]\nregel een\nregel twee\n\nzonder naam\n\n[Chorus]\nrefrein\n\n[Alleen een naam]\n\n[Bridge]\nbrug";
        const QStringList slides = ServiceModel::splitLabeled(text, &labels);
        QCOMPARE(slides, (QStringList{"regel een\nregel twee", "zonder naam", "refrein", "brug"}));   // the name never ends up in the text
        QCOMPARE(labels, (QStringList{"Verse 1", "", "Chorus", "Bridge"}));                          // one name per slide
        // joining gives text the editor can show; splitting that again gives the same slides and names
        const QString joined = ServiceModel::joinLabeled(slides, labels);
        QStringList again;
        QCOMPARE(ServiceModel::splitLabeled(joined, &again), slides);
        QCOMPARE(again, labels);
        QVERIFY(joined.startsWith("[Verse 1]\nregel een"));
        // a shorter list of names is fine; plain splitting is unchanged
        QCOMPARE(ServiceModel::joinLabeled({"a", "b"}, {"X"}), QStringLiteral("[X]\na\n\nb"));
        QCOMPARE(ServiceModel::splitSlides("[x]\na\n\nb"), (QStringList{"[x]\na", "b"}));
    }
    void orderOfSongParts() {
        const QString src = "[Couplet 1]\nregel 1\n\n[Refrein]\nrefrein\n\n[Couplet 2]\nregel 2\n\nzonder naam";
        QCOMPARE(ServiceModel::parseOrder("Couplet 1, Refrein;Couplet 2\nRefrein , ,"), (QStringList{"Couplet 1", "Refrein", "Couplet 2", "Refrein"}));
        QCOMPARE(ServiceModel::joinOrder({"A", "B"}), QStringLiteral("A, B"));
        QCOMPARE(ServiceModel::partNames(src), (QStringList{"Couplet 1", "Refrein", "Couplet 2", "#4"}));   // a part without a name is #position

        QStringList slides, labels;
        ServiceModel::expandOrder(src, {"couplet 1", "REFREIN", "Couplet  2", "Refrein", "#4"}, &slides, &labels);
        QCOMPARE(slides, (QStringList{"regel 1", "refrein", "regel 2", "refrein", "zonder naam"}));         // the chorus twice, case does not matter
        QCOMPARE(labels, (QStringList{"Couplet 1", "Refrein", "Couplet 2", "Refrein", ""}));
        ServiceModel::expandOrder(src, {"Refrein", "Bestaat niet", "Couplet 1"}, &slides, &labels);         // an unknown name is skipped
        QCOMPARE(slides, (QStringList{"refrein", "regel 1"}));
        ServiceModel::expandOrder(src, {}, &slides, &labels);                                             // no order: as written
        QCOMPARE(slides.size(), 4);
        ServiceModel::expandOrder(src, {"Onbekend", "Ook niet"}, &slides, &labels);                       // nothing matches: as written, never empty
        QCOMPARE(slides.size(), 4);
        ServiceModel::expandOrder(src, {"#9", "#0"}, &slides, &labels);
        QCOMPARE(slides.size(), 4);
        // saved and loaded
        ServiceModel m;
        ServiceItem it{"song", "Lied", "T", {}, {}};
        it.source = src; it.order = {"Refrein"}; it.defaultOrder = {"Couplet 1", "Refrein"}; it.ccli = "123"; it.author = "A";
        m.append(it);
        ServiceModel copy; copy.fromJson(m.toJson());
        QCOMPARE(copy.item(0)->source, src);
        QCOMPARE(copy.item(0)->order, (QStringList{"Refrein"}));
        QCOMPARE(copy.item(0)->defaultOrder, (QStringList{"Couplet 1", "Refrein"}));
        QCOMPARE(copy.item(0)->ccli, QStringLiteral("123"));
        QCOMPARE(copy.item(0)->author, QStringLiteral("A"));
    }
    void slideNamesAreSaved() {
        ServiceModel m;
        ServiceItem it{"song", "Lied", "T", {}, {"a", "b"}};
        it.labels = {"Verse 1", "Chorus"};
        m.append(it);
        ServiceModel copy;
        copy.fromJson(m.toJson());
        QCOMPARE(copy.item(0)->labels, (QStringList{"Verse 1", "Chorus"}));
        ServiceModel old;                                         // a file from before the names existed
        old.fromJson(QJsonArray{QJsonObject{{"kind", "song"}, {"title", "T"}, {"slides", QJsonArray{"a"}}}});
        QVERIFY(old.item(0)->labels.isEmpty());
    }
    void replaceKeepsTemplateWhenSet() {
        ServiceModel m;
        m.append({"custom", "T", "", {}, {"a"}, {}, "builtin:caps"});
        ServiceItem it = *m.item(0);
        it.title = "T2";
        m.replace(0, it);
        QCOMPARE(m.item(0)->templateId, QStringLiteral("builtin:caps"));
    }
};
QTEST_GUILESS_MAIN(TstService)
#include "tst_service.moc"
