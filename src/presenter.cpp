#include "presenter.h"
#include <QDateTime>
#include <QDesktopServices>
#include <algorithm>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>

Presenter::Presenter(QObject *parent) : QObject(parent) {
    m_songFilter.setSourceModel(&m_songs);      // the song list that the user sees: sorted and searchable
    QSettings s;
    m_outputScreen = s.value("outputScreen").toString();
    m_defaultBg = s.value("defaultBackground", m_defaultBg).toString();
    for (const QString &f : s.value("media").toStringList())
        if (QFileInfo::exists(QUrl(f).toLocalFile())) m_media << f;

    m_loopTimer.setSingleShot(false);
    connect(&m_loopTimer, &QTimer::timeout, this, &Presenter::loopTick);
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(600);
    connect(&m_saveTimer, &QTimer::timeout, this, &Presenter::save);

    load();
    // Template edits / deletions change what is shown right now
    connect(&m_templates, &TemplateLibrary::changed, this, &Presenter::liveChanged);
    connect(&m_templates, &TemplateLibrary::activeChanged, this, &Presenter::liveChanged);
    connect(&m_templates, &TemplateLibrary::kindDefaultsChanged, this, &Presenter::liveChanged);
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
    if (it->kind == "video") return {};                                   // a video shows no text
    if (it->slides.isEmpty()) return (slide == 0 && it->images.isEmpty()) ? it->title : QString();
    return (slide >= 0 && slide < it->slides.size()) ? it->slides[slide] : QString();
}

QString Presenter::imageAt(int item, int slide) const {
    const ServiceItem *it = m_service.item(item);
    return (it && slide >= 0 && slide < it->images.size()) ? it->images[slide] : QString();
}

QString Presenter::refAt(int item, int slide) const {
    const ServiceItem *it = m_service.item(item);
    if (!it) return {};
    if (slide >= 0 && slide < it->refs.size()) return it->refs[slide];
    return (it->kind == "scripture") ? it->subtitle : QString();
}

QVariantMap Presenter::templateFor(int item) const {
    const ServiceItem *it = m_service.item(item);
    if (it && !it->templateId.isEmpty() && m_templates.contains(it->templateId)) return m_templates.get(it->templateId);   // own choice
    if (it) {
        const QString kindId = m_templates.kindTemplate(it->kind);                                                         // default for this kind of part
        if (!kindId.isEmpty()) return m_templates.get(kindId);
    }
    return m_templates.active();   // otherwise the active template (also when a chosen template was deleted)
}

QString Presenter::backgroundOf(int item) const {
    const ServiceItem *it = m_service.item(item);
    return (it && !it->background.isEmpty()) ? it->background : m_defaultBg;
}

int Presenter::slideCount() const {
    const ServiceItem *it = m_service.item(m_item);
    return it ? std::max({1, int(it->slides.size()), int(it->images.size())}) : 0;
}

QString Presenter::liveText() const { return slideAt(m_item, m_slide); }
QString Presenter::liveImage() const { return imageAt(m_item, m_slide); }
QString Presenter::currentKind() const { const ServiceItem *it = m_service.item(m_item); return it ? it->kind : QString(); }
int Presenter::currentLoopSeconds() const { const ServiceItem *it = m_service.item(m_item); return it ? it->loopSeconds : 0; }
QString Presenter::upcomingVideo() const {
    const ServiceItem *cur = m_service.item(m_item), *next = m_service.item(m_item + 1);
    return (cur && cur->kind != "video" && next && next->kind == "video") ? next->video : QString();
}

QString Presenter::logPath() const { return dataDir() + "/log.txt"; }

void Presenter::log(const QString &message) const {
    QFile f(logPath());
    if (f.size() > 256 * 1024) {                       // keep the log small: one old file is enough
        QFile::remove(dataDir() + "/log.old.txt");
        f.rename(dataDir() + "/log.old.txt");
    }
    if (f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        f.write((QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz") + "  " + message + "\n").toUtf8());
}

void Presenter::openLogFolder() const { QDesktopServices::openUrl(QUrl::fromLocalFile(dataDir())); }

QString Presenter::liveVideo() const { const ServiceItem *it = m_service.item(m_item); return (it && it->kind == "video") ? it->video : QString(); }
bool Presenter::liveVideoLoop() const { const ServiceItem *it = m_service.item(m_item); return it && it->videoLoop; }
bool Presenter::liveVideoNext() const { const ServiceItem *it = m_service.item(m_item); return it && it->videoNext; }

QStringList Presenter::currentImages() const {
    const ServiceItem *it = m_service.item(m_item);
    if (!it) return {};
    QStringList imgs = it->images;
    while (imgs.size() < currentSlides().size()) imgs << QString();     // text slides have no image
    return imgs;
}
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

QStringList Presenter::currentSlides() const {
    const ServiceItem *it = m_service.item(m_item);
    if (!it) return {};
    return it->slides.isEmpty() ? QStringList{it->title} : it->slides;
}

QStringList Presenter::currentRefs() const {
    QStringList refs;
    for (int i = 0, n = currentSlides().size(); i < n; ++i) refs << refAt(m_item, i);
    return refs;
}

QStringList Presenter::currentLabels() const {
    const ServiceItem *it = m_service.item(m_item);
    QStringList out;
    for (int i = 0, n = currentSlides().size(); i < n; ++i) out << (it && i < it->labels.size() ? it->labels[i] : QString());
    return out;
}

QString Presenter::currentTitle() const {
    const ServiceItem *it = m_service.item(m_item);
    if (!it) return {};
    return it->subtitle.isEmpty() ? it->title : it->title + QStringLiteral(" – ") + it->subtitle;
}

void Presenter::selectItem(int i) {
    if (!m_service.item(i)) return;
    const bool otherItem = i != m_item;
    m_item = i;
    m_slide = 0;
    setTextHidden(false);                     // a new slide shows its text again
    announceItemChange(otherItem);
    noteLive();
    emit liveChanged();
    updateLoop();
}

// Called after the live part changed (or was selected again).
void Presenter::announceItemChange(bool otherItem) {
    if (otherItem) emit slidesChanged();
    if (liveVideo() != QString()) emit videoStart();   // a video always starts from the beginning when it goes live
}

void Presenter::noteLive() {
    if (!m_outputOn) return;                                   // rehearsing with the output closed does not count
    const ServiceItem *it = m_service.item(m_item);
    if (!it || it->kind != "song") return;
    const QString title = it->subtitle.isEmpty() ? it->title : it->subtitle;   // a song part is "Lied" with the song title as subtitle
    const QString key = it->ccli.isEmpty() ? "t:" + SongFilter::fold(title) : "c:" + it->ccli;
    if (m_usedThisSession.contains(key)) return;
    m_usedThisSession.insert(key);
    m_usage.record(title, it->ccli, it->author, m_name);
}

void Presenter::setLoopPaused(bool paused) {
    if (paused == m_loopPaused) return;
    m_loopPaused = paused;
    emit loopChanged();
    updateLoop();
}

void Presenter::updateLoop() {
    const ServiceItem *it = m_service.item(m_item);
    if (it && it->loopSeconds > 0 && !m_loopPaused && slideCount() > 1) m_loopTimer.start(it->loopSeconds * 1000);
    else m_loopTimer.stop();
}

void Presenter::loopTick() {
    if (slideCount() < 2) return;
    m_slide = (m_slide + 1) % slideCount();     // the automatic loop wraps around; manual "next" leaves the part
    setTextHidden(false);
    emit liveChanged();
}

void Presenter::goToSlide(int slide) {
    if (slide < 0 || slide >= slideCount()) return;
    if (slide == m_slide) {          // the slide that is already live: just bring the text back
        setTextHidden(false);
        return;
    }
    m_slide = slide;
    setTextHidden(false);
    emit liveChanged();
    updateLoop();    // a manual jump restarts the interval
}

void Presenter::nextSlide() {
    bool otherItem = false;
    if (m_slide + 1 < slideCount()) {
        ++m_slide;
    } else if (m_service.item(m_item + 1)) {
        ++m_item;
        m_slide = 0;
        otherItem = true;
    } else {
        return;
    }
    setTextHidden(false);
    announceItemChange(otherItem);
    noteLive();
    emit liveChanged();
    updateLoop();
}

void Presenter::prevSlide() {
    bool otherItem = false;
    if (m_slide > 0) {
        --m_slide;
    } else if (m_item > 0) {
        --m_item;
        m_slide = slideCount() - 1;
        otherItem = true;
    } else {
        return;
    }
    setTextHidden(false);
    announceItemChange(otherItem);
    noteLive();
    emit liveChanged();
    updateLoop();
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
    emit slidesChanged();
    emit liveChanged();
    updateLoop();
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
void Presenter::setTextHidden(bool hidden) {
    if (hidden == m_textHidden) return;
    m_textHidden = hidden;
    emit textHiddenChanged();
}

void Presenter::setLogo(bool b) { if (b != m_logo) { m_logo = b; emit logoChanged(); } }
void Presenter::setOutputOn(bool b) {
    if (b == m_outputOn) return;
    m_outputOn = b;
    emit outputOnChanged();
    m_extraScreens.setOutputOn(b);      // the extra screens that open with the output follow
    if (b) { m_usedThisSession.clear(); noteLive(); }   // a new time of showing: what is live now counts
}

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

int Presenter::addItem(const QString &kind, const QString &title, const QString &subtitle, const QString &text) {
    m_service.append({kind, title, subtitle, {}, ServiceModel::splitSlides(text)});
    return m_service.count() - 1;
}

QVariantMap Presenter::itemInfo(int i) const {
    const ServiceItem *it = m_service.item(i);
    if (!it) return {};
    return {{"kind", it->kind}, {"title", it->title}, {"subtitle", it->subtitle}, {"background", it->background},
            {"templateId", it->templateId}, {"slides", it->slides}, {"refs", it->refs}, {"images", it->images},
            {"loopSeconds", it->loopSeconds}, {"video", it->video}, {"videoLoop", it->videoLoop}, {"videoNext", it->videoNext}};
}

void Presenter::editItemSlides(int i, const QString &title, const QString &subtitle, const QStringList &slides,
                               const QStringList &images, int loopSeconds, bool keepRefs) {
    const ServiceItem *old = m_service.item(i);
    if (!old) return;
    ServiceItem it = *old;
    it.title = title;
    it.subtitle = subtitle;
    if (!(keepRefs && slides.size() == it.refs.size())) it.refs.clear();
    it.slides = slides;
    it.labels.clear();                   // the screen editor is not used for songs; slide names would no longer match
    it.source.clear(); it.order.clear(); it.defaultOrder.clear();
    it.images = images;
    while (it.images.size() > it.slides.size()) it.slides << QString();   // keep both lists the same length
    while (it.slides.size() > it.images.size() && !it.images.isEmpty()) it.images << QString();
    it.loopSeconds = qMax(0, loopSeconds);
    m_service.replace(i, it);
}

int Presenter::addSlidesItem(const QString &kind, const QString &title, const QStringList &images, int loopSeconds) {
    ServiceItem it{kind, title, "", {}, QStringList(), {}, {}};
    it.images = images;
    for (int n = 0; n < images.size(); ++n) it.slides << QString();
    it.loopSeconds = qMax(0, loopSeconds);
    m_service.append(it);
    return m_service.count() - 1;
}

int Presenter::addVideo(const QUrl &file) {
    ServiceItem it{"video", QFileInfo(file.toLocalFile()).completeBaseName(), "", {}, {}, {}, {}};
    if (it.title.isEmpty()) it.title = QStringLiteral("Video");
    it.video = file.toString();
    m_service.append(it);
    return m_service.count() - 1;
}

void Presenter::editVideo(int i, const QString &title, const QUrl &file, bool loop, bool next) {
    const ServiceItem *old = m_service.item(i);
    if (!old || old->kind != "video") return;
    ServiceItem it = *old;
    it.title = title.isEmpty() ? it.title : title;
    if (file.isValid() && !file.isEmpty()) it.video = file.toString();
    it.videoLoop = loop;
    it.videoNext = next;
    m_service.replace(i, it);
}

bool Presenter::fileExists(const QString &fileUrl) const {
    return !fileUrl.isEmpty() && QFileInfo::exists(QUrl(fileUrl).toLocalFile());
}

void Presenter::addSong(int songIndex) {
    const QModelIndex idx = m_songs.index(songIndex);
    if (!idx.isValid()) return;
    const QString number = m_songs.data(idx, SongLibrary::NumberRole).toString();
    const QString title = m_songs.data(idx, SongLibrary::TitleRole).toString();
    ServiceItem it{"song", number.isEmpty() ? QStringLiteral("Lied") : QStringLiteral("Lied %1").arg(number), title, {}, {}};
    it.source = m_songs.data(idx, SongLibrary::LyricsRole).toString();
    it.defaultOrder = ServiceModel::parseOrder(m_songs.data(idx, SongLibrary::OrderRole).toString());
    it.order = it.defaultOrder;                                  // a new part starts with the standard order of the song
    it.ccli = m_songs.data(idx, SongLibrary::CcliRole).toString();
    it.author = m_songs.data(idx, SongLibrary::AuthorRole).toString();
    ServiceModel::expandOrder(it.source, it.order, &it.slides, &it.labels);
    m_service.append(it);
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

void Presenter::editItem(int i, const QString &title, const QString &subtitle, const QString &text, const QString &order) {
    const ServiceItem *old = m_service.item(i);
    if (!old) return;
    ServiceItem it = *old;
    it.title = title;
    it.subtitle = subtitle;
    it.source = text;
    it.order = ServiceModel::parseOrder(order);
    ServiceModel::expandOrder(it.source, it.order, &it.slides, &it.labels);
    if (it.slides.size() != it.refs.size()) it.refs.clear();  // references only stay valid while slide count is unchanged
    m_service.replace(i, it);
}

QVariantList Presenter::songPreview(const QString &lyrics, const QString &order) const { return ServiceModel::preview(lyrics, order); }

QString Presenter::itemOrder(int i) const {
    const ServiceItem *it = m_service.item(i);
    return it ? ServiceModel::joinOrder(it->order) : QString();
}

QString Presenter::itemDefaultOrder(int i) const {
    const ServiceItem *it = m_service.item(i);
    return it ? ServiceModel::joinOrder(it->defaultOrder) : QString();
}

QString Presenter::itemText(int i) const {
    const ServiceItem *it = m_service.item(i);
    if (!it) return QString();
    return it->source.isEmpty() ? ServiceModel::joinLabeled(it->slides, it->labels) : it->source;   // the parts once; the order is separate
}

void Presenter::removeItem(int i) { m_service.remove(i); }
void Presenter::moveItem(int i, int delta) { moveItemTo(i, i + delta); }

void Presenter::moveItemTo(int from, int to) {
    if (!m_service.item(from) || !m_service.item(to) || from == to) return;
    // the part that is live stays live, wherever it ends up in the list
    if (m_item == from) m_item = to;
    else if (from < m_item && m_item <= to) --m_item;
    else if (to <= m_item && m_item < from) ++m_item;
    m_service.move(from, to);
}
void Presenter::setItemBackground(int i, const QString &bg) { m_service.setBackground(i, bg); }
void Presenter::setItemTemplate(int i, const QString &templateId) { m_service.setTemplate(i, templateId); }
QString Presenter::defaultTemplateName(int i) const {
    const ServiceItem *it = m_service.item(i);
    const QString kindId = it ? m_templates.kindTemplate(it->kind) : QString();
    return (kindId.isEmpty() ? m_templates.active() : m_templates.get(kindId))["name"].toString();
}

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

void Presenter::removeMedia(const QString &url) {
    if (!m_media.contains(url)) return;
    m_media.removeAll(url);
    QSettings().setValue("media", m_media);
    const QString path = QUrl(url).toLocalFile();
    if (!path.isEmpty() && path.startsWith(dataDir() + "/media/")) QFile::remove(path);   // only our own copy
    for (int i = 0; i < m_service.count(); ++i)
        if (m_service.item(i)->background == url) m_service.setBackground(i, QString());   // back to the default
    if (m_defaultBg == url) setDefaultBackground(QStringLiteral("g:sunset"));
    emit mediaFilesChanged();
    emit liveChanged();
}

QVariantMap Presenter::mediaInfo(const QString &url) const {
    static const QStringList videoExt = {"mp4", "m4v", "mov", "webm", "mkv", "avi", "wmv"};
    const QFileInfo fi(QUrl(url).toLocalFile());
    return {{"name", fi.fileName()}, {"kind", videoExt.contains(fi.suffix().toLower()) ? "video" : "image"},
            {"size", fi.size()}, {"sizeText", QLocale().formattedDataSize(fi.size(), 1)}, {"exists", fi.exists()}};
}

QVariantMap Presenter::mediaUsage(const QString &url) const {
    int parts = 0;
    for (int i = 0; i < m_service.count(); ++i) {
        const ServiceItem *it = m_service.item(i);
        if (it->background == url || (it->kind == "video" && it->video == url)) ++parts;
    }
    return {{"parts", parts}, {"isDefault", m_defaultBg == url}};
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

bool Presenter::openSongStudio() {
    // The song program sits next to this program (in the install folder, or next to this app in the folder of the disk image).
    const QString dir = QCoreApplication::applicationDirPath();
#if defined(Q_OS_WIN)
    const QString exe = dir + "/beaconview-songs.exe";
    if (!QFile::exists(exe)) return false;
    return QProcess::startDetached(exe, {});
#elif defined(Q_OS_MACOS)
    const QString app = QDir(dir + "/../../..").absoluteFilePath("BeaconView Liederen.app");
    if (!QFile::exists(app)) return false;
    return QProcess::startDetached("open", {app});
#else
    const QString exe = dir + "/beaconview-songs";
    if (!QFile::exists(exe)) return false;
    return QProcess::startDetached(exe, {});
#endif
}
