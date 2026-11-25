#include "updatechecker.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSettings>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent), m_apiUrl(QStringLiteral("https://api.github.com/repos/gndeboer89/BeaconView/releases?per_page=15")) {}

QString UpdateChecker::currentVersion() const { return QCoreApplication::applicationVersion(); }

QString UpdateChecker::platformName() {
#if defined(Q_OS_WIN)
    return "windows";
#elif defined(Q_OS_MACOS)
    return "macos";
#else
    return "linux";
#endif
}

QString UpdateChecker::normalizeVersion(const QString &tag) {
    static const QRegularExpression re(R"(^[vV]?(\d+(?:\.\d+){0,3})$)");
    const auto m = re.match(tag.trimmed());
    return m.hasMatch() ? m.captured(1) : QString();
}

int UpdateChecker::compareVersions(const QString &a, const QString &b) {
    const QStringList pa = normalizeVersion(a).split('.', Qt::SkipEmptyParts), pb = normalizeVersion(b).split('.', Qt::SkipEmptyParts);
    for (int i = 0; i < qMax(pa.size(), pb.size()); ++i) {
        const int x = i < pa.size() ? pa[i].toInt() : 0, y = i < pb.size() ? pb[i].toInt() : 0;
        if (x != y) return x > y ? 1 : -1;
    }
    return 0;
}

UpdateChecker::Result UpdateChecker::evaluate(const QByteArray &json, const QString &platform) {
    Result best;
    const QJsonArray list = QJsonDocument::fromJson(json).array();
    // While every release is marked "prerelease" they all count. As soon as there is a normal release, only normal releases
    // count: the prereleases are then test versions that the congregation's computers are not told about.
    bool hasStable = false;
    for (const auto &v : list) {
        const QJsonObject o = v.toObject();
        if (!o["draft"].toBool() && !o["prerelease"].toBool() && !normalizeVersion(o["tag_name"].toString()).isEmpty()) hasStable = true;
    }
    for (const auto &v : list) {
        const QJsonObject o = v.toObject();
        if (o["draft"].toBool()) continue;
        if (hasStable && o["prerelease"].toBool()) continue;
        const QString ver = normalizeVersion(o["tag_name"].toString());
        if (ver.isEmpty() || (best.ok && compareVersions(ver, best.version) <= 0)) continue;
        best.ok = true;
        best.version = ver;
        best.pageUrl = o["html_url"].toString();
        best.downloadUrl = best.pageUrl;
        QString setup, zip, dmg;
        for (const auto &a : o["assets"].toArray()) {
            const QString name = a.toObject()["name"].toString(), url = a.toObject()["browser_download_url"].toString();
            if (name.endsWith(".exe", Qt::CaseInsensitive) && name.contains("Setup", Qt::CaseInsensitive)) setup = url;
            else if (name.endsWith(".zip", Qt::CaseInsensitive) && name.contains("Windows", Qt::CaseInsensitive)) zip = url;
            else if (name.endsWith(".dmg", Qt::CaseInsensitive)) dmg = url;
        }
        if (platform == "windows") { if (!setup.isEmpty()) best.downloadUrl = setup; else if (!zip.isEmpty()) best.downloadUrl = zip; }
        else if (platform == "macos") { if (!dmg.isEmpty()) best.downloadUrl = dmg; }
        // linux: there is no package, the release page is the place to go
    }
    return best;
}

bool UpdateChecker::autoCheck() const { return QSettings().value("updates/auto", true).toBool(); }

void UpdateChecker::setAutoCheck(bool on) {
    if (on == autoCheck()) return;
    QSettings().setValue("updates/auto", on);
    emit autoCheckChanged();
}

QString UpdateChecker::skippedVersion() const { return QSettings().value("updates/skipped").toString(); }

void UpdateChecker::skipVersion() {
    QSettings().setValue("updates/skipped", m_result.version);
    emit changed();
}

void UpdateChecker::closeBanner() { m_closed = true; emit changed(); }

void UpdateChecker::openDownload() const {
    if (m_result.ok) QDesktopServices::openUrl(QUrl(m_result.downloadUrl));
}

void UpdateChecker::checkIfDue(bool outputOpen, int hours) {
    if (!autoCheck() || outputOpen || m_checking) return;
    const QDateTime last = QSettings().value("updates/last").toDateTime();
    if (last.isValid() && last.secsTo(QDateTime::currentDateTime()) < hours * 3600) return;
    check(false);
}

void UpdateChecker::check(bool manual) {
    if (m_checking) return;
    m_checking = true;
    if (manual) m_message = QStringLiteral("Bezig met controleren…");
    emit changed();
    QNetworkRequest req(m_apiUrl);
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("User-Agent", "BeaconView/" + currentVersion().toUtf8());          // the program and its version, nothing else
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(10000);
    QNetworkReply *reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, manual] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) { finish({}, reply->errorString(), manual); return; }
        const Result r = evaluate(reply->readAll(), platformName());
        finish(r, r.ok ? QString() : QStringLiteral("Geen versie gevonden"), manual);
    });
}

void UpdateChecker::finish(const Result &r, const QString &error, bool manual) {
    m_checking = false;
    if (error.isEmpty()) {
        m_result = r;
        m_closed = false;
        QSettings().setValue("updates/last", QDateTime::currentDateTime());
        if (manual) m_message = available() ? QStringLiteral("Versie %1 is beschikbaar.").arg(r.version)
                                            : QStringLiteral("Je hebt de nieuwste versie (%1).").arg(currentVersion());
    } else if (manual) {
        m_message = QStringLiteral("Controleren lukt nu niet: %1").arg(error);
    }                                                      // an automatic check that fails says nothing
    emit changed();
}
