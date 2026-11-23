#include <QtTest>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "templatelibrary.h"

class TstTemplates : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("BeaconViewTest");
        QCoreApplication::setApplicationName("tst_templates");
        QStandardPaths::setTestModeEnabled(true);
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).removeRecursively();
        QSettings().clear();
    }
    void builtinsAreReadOnly() {
        TemplateLibrary lib;
        QVERIFY(lib.all().size() >= 5);
        QCOMPARE(lib.activeId(), QStringLiteral("builtin:classic"));
        lib.update("builtin:classic", {{"fontSize", 50}});          // ignored
        QCOMPARE(lib.get("builtin:classic")["fontSize"].toInt(), 108);
        lib.remove("builtin:classic");                               // ignored
        QVERIFY(lib.all().size() >= 5);
    }
    void customLifecycle() {
        TemplateLibrary lib;
        const int before = lib.all().size();
        const QString id = lib.duplicate("builtin:elegant");
        QVERIFY(id.startsWith("custom:"));
        QCOMPARE(lib.all().size(), before + 1);
        QVERIFY(lib.get(id)["name"].toString().endsWith("(kopie)"));
        QVERIFY(!lib.get(id)["builtin"].toBool());

        lib.update(id, {{"fontSize", 77}, {"textColor", "#123456"}, {"id", "hack"}, {"builtin", true}});
        QCOMPARE(lib.get(id)["fontSize"].toInt(), 77);
        QCOMPARE(lib.get(id)["textColor"].toString(), QStringLiteral("#123456"));
        QCOMPARE(lib.get(id)["id"].toString(), id);                  // id/builtin cannot be overwritten
        QVERIFY(!lib.get(id)["builtin"].toBool());

        TemplateLibrary reloaded;                                    // persisted on disk
        QCOMPARE(reloaded.get(id)["fontSize"].toInt(), 77);

        lib.setActive(id);
        QCOMPARE(lib.active()["id"].toString(), id);
        lib.remove(id);
        QCOMPARE(lib.all().size(), before);
        QCOMPARE(lib.activeId(), QStringLiteral("builtin:classic")); // falls back when active one is removed
    }
    void contains() {
        TemplateLibrary lib;
        QVERIFY(lib.contains("builtin:modern"));
        QVERIFY(!lib.contains("custom:gone"));
        QVERIFY(!lib.contains(""));
    }
    void kindDefaults() {
        QSettings().remove("kindTemplates");
        TemplateLibrary lib;
        QCOMPARE(lib.kindTemplate("song"), QString());
        QSignalSpy spy(&lib, &TemplateLibrary::kindDefaultsChanged);
        lib.setKindTemplate("song", "builtin:modern");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(lib.kindTemplate("song"), QStringLiteral("builtin:modern"));
        QCOMPARE(lib.kindTemplate("prayer"), QString());
        lib.setKindTemplate("song", "builtin:modern");              // unchanged: no signal
        QCOMPARE(spy.count(), 1);
        lib.setKindTemplate("song", "does-not-exist");              // unknown id clears the default
        QCOMPARE(lib.kindTemplate("song"), QString());
        lib.setKindTemplate("song", "builtin:caps");

        TemplateLibrary again;                                       // persisted
        QCOMPARE(again.kindTemplate("song"), QStringLiteral("builtin:caps"));

        const QString id = lib.duplicate("builtin:elegant");         // deleting a template clears defaults using it
        lib.setKindTemplate("scripture", id);
        QCOMPARE(lib.kindTemplate("scripture"), id);
        lib.remove(id);
        QCOMPARE(lib.kindTemplate("scripture"), QString());
        TemplateLibrary third;
        QCOMPARE(third.kindTemplate("scripture"), QString());
        QCOMPARE(third.kindTemplate("song"), QStringLiteral("builtin:caps"));
        lib.setKindTemplate("song", "");
        QCOMPARE(lib.kindTemplate("song"), QString());
    }
    void unknownIdFallsBack() {
        TemplateLibrary lib;
        QCOMPARE(lib.get("nope")["id"].toString(), QStringLiteral("builtin:classic"));
        QVERIFY(lib.get("nope").contains("textColor"));              // always complete
    }
};
QTEST_GUILESS_MAIN(TstTemplates)
#include "tst_templates.moc"
