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
