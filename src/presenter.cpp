#include "presenter.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>

Presenter::Presenter(QObject *parent) : QObject(parent) {
    QSettings s;
    m_outputScreen = s.value("outputScreen").toString();
    m_defaultBg = s.value("defaultBackground", m_defaultBg).toString();
    for (const QString &f : s.value("media").toStringList())
        if (QFileInfo::exists(QUrl(f).toLocalFile())) m_media << f;

    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(600);
    connect(&m_saveTimer, &QTimer::timeout, this, &Presenter::save);

    load();
    // Template edits / deletions change what is shown right now
    connect(&m_templates, &TemplateLibrary::changed, this, &Presenter::liveChanged);
    connect(&m_templates, &TemplateLibrary::activeChanged, this, &Presenter::liveChanged);
    // Keep indices valid and the live view fresh whenever the service changes.
    connect(&m_service, &QAbstractItemModel::dataChanged, this, &Presenter::onModelChanged);
    connect(&m_service, &QAbstractItemModel::rowsInserted, this, &Presenter::onModelChanged);
    connect(&m_service, &QAbstractItemModel::rowsRemoved, this, &Presenter::onModelChanged);
    connect(&m_service, &QAbstractItemModel::rowsMoved, this, &Presenter::onModelChanged);
    connect(&m_service, &QAbstractItemModel::modelReset, this, &Presenter::onModelChanged);
}

QString Presenter::dataDir() const {
    const QString d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(d);
    return d;
}

// ---- live state -----------------------------------------------------------

QString Presenter::slideAt(int item, int slide) const {
    const ServiceItem *it = m_service.item(item);
    if (!it) return {};
    if (it->slides.isEmpty()) return slide == 0 ? it->title : QString();
    return (slide >= 0 && slide < it->slides.size()) ? it->slides[slide] : QString();
}

QString Presenter::refAt(int item, int slide) const {
    const ServiceItem *it = m_service.item(item);
    if (!it) return {};
    if (slide >= 0 && slide < it->refs.size()) return it->refs[slide];
    return (it->kind == "scripture") ? it->subtitle : QString();
}

QVariantMap Presenter::templateFor(int item) const {
    const ServiceItem *it = m_service.item(item);
    if (it && !it->templateId.isEmpty() && m_templates.contains(it->templateId)) return m_templates.get(it->templateId);
    return m_templates.active();   // no own template, or the chosen one was deleted
}

QString Presenter::backgroundOf(int item) const {
    const ServiceItem *it = m_service.item(item);
    return (it && !it->background.isEmpty()) ? it->background : m_defaultBg;
}

int Presenter::slideCount() const {
    const ServiceItem *it = m_service.item(m_item);
    return it ? qMax(1, int(it->slides.size())) : 0;
}

QString Presenter::liveText() const { return slideAt(m_item, m_slide); }
QString Presenter::liveReference() const { return refAt(m_item, m_slide); }
QString Presenter::liveBackground() const { return backgroundOf(m_item); }

QString Presenter::nextText() const {
    if (m_slide + 1 < slideCount()) return slideAt(m_item, m_slide + 1);
    return slideAt(m_item + 1, 0);
}

QString Presenter::nextReference() const {
    if (m_slide + 1 < slideCount()) return refAt(m_item, m_slide + 1);
    return refAt(m_item + 1, 0);
}

QVariantMap Presenter::liveTemplate() const { return templateFor(m_item); }
QVariantMap Presenter::nextTemplate() const { return templateFor(m_slide + 1 < slideCount() ? m_item : m_item + 1); }

QString Presenter::nextBackground() const {
    return backgroundOf(m_slide + 1 < slideCount() ? m_item : m_item + 1);
}

void Presenter::selectItem(int i) {
    if (!m_service.item(i)) return;
    m_item = i;
    m_slide = 0;
    emit liveChanged();
}

void Presenter::nextSlide() {
    if (m_slide + 1 < slideCount()) {
        ++m_slide;
    } else if (m_service.item(m_item + 1)) {
        ++m_item;
        m_slide = 0;
    } else {
        return;
    }
    emit liveChanged();
}

void Presenter::prevSlide() {
    if (m_slide > 0) {
        --m_slide;
    } else if (m_item > 0) {
        --m_item;
        m_slide = slideCount() - 1;
    } else {
        return;
    }
    emit liveChanged();
}

void Presenter::startService() {
    m_blackout = false;
    emit blackoutChanged();
    setOutputOn(true);
    selectItem(0);
}

void Presenter::onModelChanged() {
    if (m_item >= m_service.count()) m_item = qMax(0, m_service.count() - 1);
    if (m_slide >= slideCount()) m_slide = qMax(0, slideCount() - 1);
    emit liveChanged();
    scheduleSave();
}

// ---- toggles / settings ---------------------------------------------------

void Presenter::setServiceName(const QString &n) {
    if (n == m_name) return;
    m_name = n;
    emit serviceNameChanged();
    scheduleSave();
}
void Presenter::setBlackout(bool b) { if (b != m_blackout) { m_blackout = b; emit blackoutChanged(); } }
void Presenter::setLogo(bool b) { if (b != m_logo) { m_logo = b; emit logoChanged(); } }
void Presenter::setOutputOn(bool b) { if (b != m_outputOn) { m_outputOn = b; emit outputOnChanged(); } }

void Presenter::setOutputScreen(const QString &s) {
    if (s == m_outputScreen) return;
    m_outputScreen = s;
    QSettings().setValue("outputScreen", s);
    emit outputScreenChanged();
}

void Presenter::setDefaultBackground(const QString &b) {
    if (b == m_defaultBg) return;
    m_defaultBg = b;
    QSettings().setValue("defaultBackground", b);
    emit defaultBackgroundChanged();
    emit liveChanged();
}

// ---- editing --------------------------------------------------------------

void Presenter::addItem(const QString &kind, const QString &title, const QString &subtitle, const QString &text) {
    m_service.append({kind, title, subtitle, {}, ServiceModel::splitSlides(text)});
}

void Presenter::addSong(int songIndex) {
    const QModelIndex idx = m_songs.index(songIndex);
    if (!idx.isValid()) return;
    const QString number = m_songs.data(idx, SongLibrary::NumberRole).toString();
    const QString title = m_songs.data(idx, SongLibrary::TitleRole).toString();
    m_service.append({"song", number.isEmpty() ? QStringLiteral("Lied") : QStringLiteral("Lied %1").arg(number), title, {},
                      ServiceModel::splitSlides(m_songs.data(idx, SongLibrary::LyricsRole).toString())});
}

int Presenter::addPassage(const QString &reference, int versesPerSlide, bool verseNumbers, bool show) {
    const Bible::Passage p = m_bible.parse(reference);
    const auto slides = m_bible.slides(p, versesPerSlide, verseNumbers);
    if (slides.isEmpty()) return -1;
    ServiceItem it{"scripture", "Schriftlezing", m_bible.label(p), {}, {}, {}};
    for (const auto &s : slides) { it.slides << s.text; it.refs << s.reference; }
    m_service.append(it);
    const int idx = m_service.count() - 1;
    if (show) selectItem(idx);
    return idx;
}

void Presenter::editItem(int i, const QString &title, const QString &subtitle, const QString &text) {
    const ServiceItem *old = m_service.item(i);
    if (!old) return;
    ServiceItem it = *old;
    it.title = title;
    it.subtitle = subtitle;
    it.slides = ServiceModel::splitSlides(text);
    if (it.slides.size() != it.refs.size()) it.refs.clear();  // references only stay valid while slide count is unchanged
    m_service.replace(i, it);
}

QString Presenter::itemText(int i) const {
    const ServiceItem *it = m_service.item(i);
    return it ? it->slides.join("\n\n") : QString();
}

void Presenter::removeItem(int i) { m_service.remove(i); }
void Presenter::moveItem(int i, int delta) { m_service.move(i, i + delta); }
void Presenter::setItemBackground(int i, const QString &bg) { m_service.setBackground(i, bg); }
void Presenter::setItemTemplate(int i, const QString &templateId) { m_service.setTemplate(i, templateId); }
QString Presenter::itemTemplate(int i) const { const ServiceItem *it = m_service.item(i); return it ? it->templateId : QString(); }

void Presenter::importMedia(const QUrl &file) {
    const QString src = file.toLocalFile();
    if (src.isEmpty()) return;
    const QString dir = dataDir() + "/media";
    QDir().mkpath(dir);
    QString dst = dir + "/" + QFileInfo(src).fileName();
    for (int n = 1; QFileInfo::exists(dst); ++n)
        dst = QStringLiteral("%1/%2-%3.%4").arg(dir, QFileInfo(src).completeBaseName()).arg(n).arg(QFileInfo(src).suffix());
    if (!QFile::copy(src, dst)) return;
    m_media << QUrl::fromLocalFile(dst).toString();
    QSettings().setValue("media", m_media);
    emit mediaFilesChanged();
}

// ---- persistence ----------------------------------------------------------

void Presenter::scheduleSave() { m_saveTimer.start(); }

void Presenter::save() {
    QFile f(dataDir() + "/service.json");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(QJsonDocument(QJsonObject{{"name", m_name}, {"items", m_service.toJson()}}).toJson());
    m_lastSaved = QDateTime::currentDateTime().toString("HH:mm:ss");
    emit savedChanged();
}

void Presenter::load() {
    QFile f(dataDir() + "/service.json");
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
        if (o.contains("items")) {
            m_name = o["name"].toString(m_name);
            m_service.fromJson(o["items"].toArray());
            return;
        }
    }
    seedDefaultService();
}

void Presenter::seedDefaultService() {
    m_service.append({"welcome", "Welkom", "", {}, {"Welkom"}});
    addSong(0);
    m_service.append({"prayer", "Gebed", "", {}, {"Gebed"}});
    m_service.append({"scripture", "Schriftlezing", "Psalm 23", {},
                      {"De HEERE is mijn Herder,\nmij zal niets ontbreken."}});
    m_service.append({"sermon", "Verkondiging", "", {}, {"Verkondiging"}});
    m_service.append({"offering", "Collecte", "", {}, {"Collecte"}});
    m_service.append({"blessing", "Zegen", "", {}, {"Zegen"}});
}
