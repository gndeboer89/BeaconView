#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include "updatechecker.h"

// One release as the GitHub API gives it; `assets` are file names (the download address is made from the tag and the name).
static QJsonObject rel(const QString &tag, bool pre, bool draft = false, const QStringList &assets = {}) {
    QJsonArray a;
    for (const QString &n : assets)
        a << QJsonObject{{"name", n}, {"browser_download_url", "https://example.org/d/" + tag + "/" + n}};
    return {{"tag_name", tag}, {"draft", draft}, {"prerelease", pre}, {"html_url", "https://example.org/r/" + tag}, {"assets", a}};
}
static QByteArray list(const QList<QJsonObject> &rels) {
    QJsonArray arr;
    for (const auto &r : rels) arr << r;
    return QJsonDocument(arr).toJson();
}
// all releases are prereleases, as they are now
static QByteArray releases() {
    return list({rel("v0.1.4.0", true, false, {"BeaconView-macOS.dmg", "BeaconView-Windows.zip", "BeaconView-Setup-0.1.4.0.exe"}),
                 rel("v0.1.10.0", true, false, {"BeaconView-Windows.zip"}),
                 rel("v0.1.9.0", true),
                 rel("v9.9.9.9", false, true),          // a draft
                 rel("nightly", true)});                // not a version number
}

class TstUpdateChecker : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("BeaconViewTest");
        QCoreApplication::setApplicationName("tst_updatechecker");
        QCoreApplication::setApplicationVersion("0.1.4.0");
        QStandardPaths::setTestModeEnabled(true);
    }
    void init() { QSettings().clear(); }

    void comparesVersions() {
        QCOMPARE(UpdateChecker::compareVersions("0.1.10.0", "0.1.9.0"), 1);          // numbers, not text
        QCOMPARE(UpdateChecker::compareVersions("v0.1.4.0", "0.1.4.0"), 0);
        QCOMPARE(UpdateChecker::compareVersions("0.1.4", "0.1.4.0"), 0);             // a missing part is 0
        QCOMPARE(UpdateChecker::compareVersions("0.2", "0.1.9.9"), 1);
        QCOMPARE(UpdateChecker::compareVersions("0.0.9.0", "0.1.0.0"), -1);
        QCOMPARE(UpdateChecker::normalizeVersion(" V1.2.3.4 "), QStringLiteral("1.2.3.4"));
        QVERIFY(UpdateChecker::normalizeVersion("nightly").isEmpty());
        QVERIFY(UpdateChecker::normalizeVersion("1.2.3.4.5").isEmpty());
        QVERIFY(UpdateChecker::normalizeVersion("").isEmpty());
    }
    void choosesTheNewestRelease() {
        const auto win = UpdateChecker::evaluate(releases(), "windows");
        QVERIFY(win.ok);
        QCOMPARE(win.version, QStringLiteral("0.1.10.0"));                           // highest number; all are prereleases, so all count;
        QCOMPARE(win.pageUrl, QStringLiteral("https://example.org/r/v0.1.10.0"));     // a draft and a name that is no version are skipped
        QCOMPARE(win.downloadUrl, QStringLiteral("https://example.org/d/v0.1.10.0/BeaconView-Windows.zip"));   // no installer in that release: the zip
        const auto mac = UpdateChecker::evaluate(releases(), "macos");
        QCOMPARE(mac.downloadUrl, mac.pageUrl);                                      // no dmg in it: the page
    }
    void normalReleasesBeatPrereleases() {
        // once there is a normal release, prereleases are test versions: not offered, even when they are newer
        const QByteArray mixed = list({rel("v0.3.0.0", true), rel("v0.2.1.0", false), rel("v0.2.0.0", false), rel("v0.1.5.0", true)});
        QCOMPARE(UpdateChecker::evaluate(mixed, "windows").version, QStringLiteral("0.2.1.0"));
        // a draft or a tag that is no version never makes a normal release
        const QByteArray fake = list({rel("v0.3.0.0", true), rel("v9.0.0.0", false, true), rel("latest", false)});
        QCOMPARE(UpdateChecker::evaluate(fake, "windows").version, QStringLiteral("0.3.0.0"));
    }
    void choosesTheFileOfThisSystem() {
        const QByteArray one = list({rel("v0.1.4.0", true, false, {"BeaconView-macOS.dmg", "BeaconView-Windows.zip", "BeaconView-Setup-0.1.4.0.exe"})});
        const QString base = "https://example.org/d/v0.1.4.0/";
        QCOMPARE(UpdateChecker::evaluate(one, "windows").downloadUrl, base + "BeaconView-Setup-0.1.4.0.exe");   // installer before zip
        QCOMPARE(UpdateChecker::evaluate(one, "macos").downloadUrl, base + "BeaconView-macOS.dmg");
        QCOMPARE(UpdateChecker::evaluate(one, "linux").downloadUrl, QStringLiteral("https://example.org/r/v0.1.4.0"));   // no package: the page
    }
    void badAnswersGiveNothing() {
        QVERIFY(!UpdateChecker::evaluate("", "windows").ok);
        QVERIFY(!UpdateChecker::evaluate("not json", "windows").ok);
        QVERIFY(!UpdateChecker::evaluate("{\"message\":\"rate limit\"}", "windows").ok);   // an error object, not a list
        QVERIFY(!UpdateChecker::evaluate("[]", "windows").ok);
    }
    void checksAFileAndReportsNewer() {
        QTemporaryDir dir;
        QFile f(dir.filePath("releases.json")); QVERIFY(f.open(QIODevice::WriteOnly)); f.write(releases()); f.close();
        UpdateChecker u;
        u.setApiUrl(QUrl::fromLocalFile(f.fileName()));
        QVERIFY(!u.available());
        QSignalSpy spy(&u, &UpdateChecker::changed);
        u.check(true);
        QTRY_VERIFY(!u.checking());
        QVERIFY(u.available());
        QCOMPARE(u.latestVersion(), QStringLiteral("0.1.10.0"));
        QVERIFY(u.bannerVisible());
        QVERIFY(u.message().contains("0.1.10.0"));
        // closing hides it until the next start; skipping hides this version for good
        u.closeBanner();
        QVERIFY(!u.bannerVisible());
        UpdateChecker again;
        again.setApiUrl(QUrl::fromLocalFile(f.fileName()));
        again.check(false);
        QTRY_VERIFY(again.available());
        QVERIFY(again.bannerVisible());
        again.skipVersion();
        QVERIFY(!again.bannerVisible());
        QVERIFY(again.available());                                                  // the settings page still knows
        UpdateChecker third;
        third.setApiUrl(QUrl::fromLocalFile(f.fileName()));
        third.check(false);
        QTRY_VERIFY(third.available());
        QVERIFY(!third.bannerVisible());                                             // still skipped after a restart
    }
    void sameVersionAndErrors() {
        QCoreApplication::setApplicationVersion("0.1.10.0");
        QTemporaryDir dir;
        QFile f(dir.filePath("releases.json")); QVERIFY(f.open(QIODevice::WriteOnly)); f.write(releases()); f.close();
        UpdateChecker u;
        u.setApiUrl(QUrl::fromLocalFile(f.fileName()));
        u.check(true);
        QTRY_VERIFY(!u.checking());
        QVERIFY(!u.available());
        QVERIFY(u.message().contains("nieuwste"));
        QCoreApplication::setApplicationVersion("0.1.4.0");
        UpdateChecker bad;
        bad.setApiUrl(QUrl::fromLocalFile(dir.filePath("bestaat-niet.json")));
        bad.check(true);                                                             // a manual check says why it failed
        QTRY_VERIFY(!bad.checking());
        QVERIFY(!bad.available());
        QVERIFY(bad.message().contains("lukt nu niet"));
        UpdateChecker quiet;
        quiet.setApiUrl(QUrl::fromLocalFile(dir.filePath("bestaat-niet.json")));
        quiet.check(false);                                                          // an automatic check says nothing
        QTRY_VERIFY(!quiet.checking());
        QVERIFY(quiet.message().isEmpty());
    }
    void automaticChecksAreRationed() {
        QTemporaryDir dir;
        QFile f(dir.filePath("releases.json")); QVERIFY(f.open(QIODevice::WriteOnly)); f.write(releases()); f.close();
        UpdateChecker u;
        u.setApiUrl(QUrl::fromLocalFile(f.fileName()));
        QVERIFY(u.autoCheck());                                                      // on by default
        u.checkIfDue(true);                                                          // something is on the output: not now
        QVERIFY(!u.checking());
        u.checkIfDue(false);
        QVERIFY(u.checking());
        QTRY_VERIFY(!u.checking());
        QVERIFY(u.available());
        UpdateChecker next;
        next.setApiUrl(QUrl::fromLocalFile(f.fileName()));
        next.checkIfDue(false);                                                      // just checked: not again within 12 hours
        QVERIFY(!next.checking());
        next.setAutoCheck(false);
        QVERIFY(!next.autoCheck());
        next.checkIfDue(false, 0);                                                   // switched off: never
        QVERIFY(!next.checking());
    }
};
QTEST_GUILESS_MAIN(TstUpdateChecker)
#include "tst_updatechecker.moc"
