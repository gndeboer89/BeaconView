#pragma once
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

// Looks on GitHub whether there is a newer version of BeaconView and tells the user. It only tells: nothing is downloaded or
// installed by the program itself. The highest version number wins. While all releases are marked "prerelease" (as they are
// now) they all count; once there is a normal release, prereleases are left out, so they can be used to test.
class UpdateChecker : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)               // a newer version exists
    Q_PROPERTY(bool bannerVisible READ bannerVisible NOTIFY changed)       // ... and it was not skipped or closed
    Q_PROPERTY(bool checking READ checking NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)                // result of the last check, for the settings page
    Q_PROPERTY(bool autoCheck READ autoCheck WRITE setAutoCheck NOTIFY autoCheckChanged)
public:
    struct Result {
        bool ok = false;           // a usable release was found
        QString version;           // "0.1.5.0"
        QString pageUrl;           // the release page
        QString downloadUrl;       // the file for this system, or the page when there is none
    };

    explicit UpdateChecker(QObject *parent = nullptr);

    QString currentVersion() const;
    QString latestVersion() const { return m_result.version; }
    bool available() const { return m_result.ok && compareVersions(m_result.version, currentVersion()) > 0; }
    bool bannerVisible() const { return available() && !m_closed && m_result.version != skippedVersion(); }
    bool checking() const { return m_checking; }
    QString message() const { return m_message; }
    bool autoCheck() const;
    void setAutoCheck(bool on);

    // Asks GitHub. `manual`: the user asked, so a result is always reported and a skipped version counts again.
    Q_INVOKABLE void check(bool manual = false);
    // Checks at most once per `hours`, and only when automatic checking is on and nothing is on the output.
    void checkIfDue(bool outputOpen, int hours = 12);
    Q_INVOKABLE void openDownload() const;
    Q_INVOKABLE void skipVersion();          // never tell about this version again
    Q_INVOKABLE void closeBanner();          // hide it until the next start

    // The address that is asked (tests point it at a file).
    void setApiUrl(const QUrl &url) { m_apiUrl = url; }
    // Reads the list of releases (the JSON of the GitHub API) and chooses the newest one that users should be told about. `platform`: "windows", "macos" or "linux".
    static Result evaluate(const QByteArray &json, const QString &platform);
    static QString platformName();
    // 1, 0 or -1; "v0.1.10.0" is more than "0.1.9.0"; a missing part counts as 0.
    static int compareVersions(const QString &a, const QString &b);
    // "v0.1.4.0" -> "0.1.4.0", "" if the text is not a version number.
    static QString normalizeVersion(const QString &tag);

signals:
    void changed();
    void autoCheckChanged();

private:
    QString skippedVersion() const;
    void finish(const Result &r, const QString &error, bool manual);

    QNetworkAccessManager m_net;
    QUrl m_apiUrl;
    Result m_result;
    bool m_checking = false, m_closed = false;
    QString m_message;
};
