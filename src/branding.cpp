#include "branding.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QSettings>
#include <QStandardPaths>
#include <algorithm>

Branding::Branding(QObject *parent) : QObject(parent) {
    QSettings s;
    m_bg = s.value("branding/bg", m_bg).toString();
    m_scale = s.value("branding/scale", m_scale).toDouble();
    m_watermark = s.value("branding/watermark", m_watermark).toBool();
    m_corner = s.value("branding/corner", m_corner).toInt();
    m_wmSize = s.value("branding/wmSize", m_wmSize).toDouble();
    const QString src = s.value("branding/source").toString();
    if (!src.isEmpty() && QFileInfo::exists(QUrl(src).toLocalFile())) m_source = src;  // forget logos that were deleted
}

QString Branding::dir() const {
    const QString d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/branding";
    QDir().mkpath(d);
    return d;
}

void Branding::removeStoredLogos() {
    const QDir d(dir());
    for (const QString &f : d.entryList({"logo-*"}, QDir::Files)) QFile::remove(d.filePath(f));
}

bool Branding::chooseLogo(const QUrl &file) {
    const QString src = file.toLocalFile();
    if (src.isEmpty() || !QImageReader(src).canRead()) return false;
    removeStoredLogos();
    // A new file name every time, so QML never shows a cached copy of the previous logo.
    const QString dst = QStringLiteral("%1/logo-%2.%3").arg(dir()).arg(QDateTime::currentMSecsSinceEpoch()).arg(QFileInfo(src).suffix().toLower());
    if (!QFile::copy(src, dst)) return false;
    m_source = QUrl::fromLocalFile(dst).toString();
    QSettings().setValue("branding/source", m_source);
    emit changed();
    return true;
}

void Branding::clearLogo() {
    removeStoredLogos();
    m_source.clear();
    QSettings().remove("branding/source");
    emit changed();
}

void Branding::setBgColor(const QString &c) { if (c != m_bg) { m_bg = c; QSettings().setValue("branding/bg", c); emit changed(); } }
void Branding::setScale(double s) { s = std::clamp(s, 0.1, 1.0); if (s != m_scale) { m_scale = s; QSettings().setValue("branding/scale", s); emit changed(); } }
void Branding::setWatermark(bool b) { if (b != m_watermark) { m_watermark = b; QSettings().setValue("branding/watermark", b); emit changed(); } }
void Branding::setCorner(int c) { c = std::clamp(c, 0, 3); if (c != m_corner) { m_corner = c; QSettings().setValue("branding/corner", c); emit changed(); } }
void Branding::setWatermarkSize(double s) { s = std::clamp(s, 0.04, 0.4); if (s != m_wmSize) { m_wmSize = s; QSettings().setValue("branding/wmSize", s); emit changed(); } }
