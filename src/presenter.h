#pragma once
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include "bible.h"
#include "extrascreens.h"
#include "branding.h"
#include "overlaycontroller.h"
#include "servicemodel.h"
#include "slideimporter.h"
#include "songfilter.h"
#include "songlibrary.h"
#include "templatelibrary.h"
#include "usagelog.h"

// Central controller: owns the service, the live state shown on the output
// screen(s) and persistence. Exposed to QML as `presenter`.
class Presenter : public QObject {
    Q_OBJECT
    Q_PROPERTY(ServiceModel *service READ service CONSTANT)
    Q_PROPERTY(SongLibrary *songs READ songs CONSTANT)
    Q_PROPERTY(SongFilter *songFilter READ songFilter CONSTANT)   // the song list: sorted and searchable
    Q_PROPERTY(ExtraScreens *extraScreens READ extraScreens CONSTANT)
    Q_PROPERTY(UsageLog *usage READ usage CONSTANT)                // which songs were shown (report)
    Q_PROPERTY(Bible *bible READ bible CONSTANT)
    Q_PROPERTY(TemplateLibrary *templates READ templates CONSTANT)
    Q_PROPERTY(OverlayController *overlays READ overlays CONSTANT)
    Q_PROPERTY(Branding *branding READ branding CONSTANT)
    Q_PROPERTY(SlideImporter *importer READ importer CONSTANT)
    Q_PROPERTY(QString serviceName READ serviceName WRITE setServiceName NOTIFY serviceNameChanged)
    Q_PROPERTY(int currentItem READ currentItem NOTIFY liveChanged)
    Q_PROPERTY(int currentSlide READ currentSlide NOTIFY liveChanged)
    Q_PROPERTY(int slideCount READ slideCount NOTIFY liveChanged)
    // All slides of the current part, so the operator can click any of them
    Q_PROPERTY(QStringList currentSlides READ currentSlides NOTIFY slidesChanged)
    Q_PROPERTY(QStringList currentRefs READ currentRefs NOTIFY slidesChanged)
    Q_PROPERTY(QStringList currentLabels READ currentLabels NOTIFY slidesChanged)   // names of the slides of a song ("Verse 1")
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY slidesChanged)
    Q_PROPERTY(QStringList currentImages READ currentImages NOTIFY slidesChanged)   // image per slide ("" = text slide)
    Q_PROPERTY(QString currentKind READ currentKind NOTIFY slidesChanged)
    Q_PROPERTY(int currentLoopSeconds READ currentLoopSeconds NOTIFY slidesChanged) // > 0: this part loops by itself
    Q_PROPERTY(bool loopPaused READ loopPaused WRITE setLoopPaused NOTIFY loopChanged)
    Q_PROPERTY(QString liveImage READ liveImage NOTIFY liveChanged)
    // The video of the part that is live ("" when the live part is not a video)
    Q_PROPERTY(QString liveVideo READ liveVideo NOTIFY slidesChanged)
    // The video that is the NEXT part (when the live part is not a video): it can be loaded in advance so it starts at once.
    Q_PROPERTY(QString upcomingVideo READ upcomingVideo NOTIFY slidesChanged)
    Q_PROPERTY(QString logPath READ logPath CONSTANT)
    Q_PROPERTY(bool liveVideoLoop READ liveVideoLoop NOTIFY slidesChanged)
    Q_PROPERTY(bool liveVideoNext READ liveVideoNext NOTIFY slidesChanged)
    Q_PROPERTY(QString liveText READ liveText NOTIFY liveChanged)
    Q_PROPERTY(QString liveBackground READ liveBackground NOTIFY liveChanged)
    Q_PROPERTY(QString liveReference READ liveReference NOTIFY liveChanged)
    Q_PROPERTY(QVariantMap liveTemplate READ liveTemplate NOTIFY liveChanged)
    Q_PROPERTY(QVariantMap nextTemplate READ nextTemplate NOTIFY liveChanged)
    Q_PROPERTY(QString nextText READ nextText NOTIFY liveChanged)
    Q_PROPERTY(QString nextReference READ nextReference NOTIFY liveChanged)
    Q_PROPERTY(QString nextBackground READ nextBackground NOTIFY liveChanged)
    Q_PROPERTY(bool blackout READ blackout WRITE setBlackout NOTIFY blackoutChanged)
    Q_PROPERTY(bool logo READ logo WRITE setLogo NOTIFY logoChanged)
    // Hides the text (and bible reference) but keeps the background; shows again on the next slide.
    Q_PROPERTY(bool textHidden READ textHidden WRITE setTextHidden NOTIFY textHiddenChanged)
    Q_PROPERTY(bool outputOn READ outputOn WRITE setOutputOn NOTIFY outputOnChanged)
    Q_PROPERTY(QString outputScreen READ outputScreen WRITE setOutputScreen NOTIFY outputScreenChanged)
    Q_PROPERTY(QString defaultBackground READ defaultBackground WRITE setDefaultBackground NOTIFY defaultBackgroundChanged)
    Q_PROPERTY(QStringList mediaFiles READ mediaFiles NOTIFY mediaFilesChanged)
    Q_PROPERTY(QString lastSaved READ lastSaved NOTIFY savedChanged)
public slots:
    void loopTick();        // announcement loop: next slide, wrapping around (called by the timer; public for tests)

public:
    explicit Presenter(QObject *parent = nullptr);

    ServiceModel *service() { return &m_service; }
    SongLibrary *songs() { return &m_songs; }
    SongFilter *songFilter() { return &m_songFilter; }
    ExtraScreens *extraScreens() { return &m_extraScreens; }
    UsageLog *usage() { return &m_usage; }
    Bible *bible() { return &m_bible; }
    TemplateLibrary *templates() { return &m_templates; }
    OverlayController *overlays() { return &m_overlays; }
    Branding *branding() { return &m_branding; }
    SlideImporter *importer() { return &m_importer; }
    QString serviceName() const { return m_name; }
    void setServiceName(const QString &n);
    int currentItem() const { return m_item; }
    int currentSlide() const { return m_slide; }
    int slideCount() const;
    QStringList currentSlides() const;
    QStringList currentRefs() const;
    QStringList currentLabels() const;
    QString currentTitle() const;
    QStringList currentImages() const;
    QString currentKind() const;
    int currentLoopSeconds() const;
    bool loopPaused() const { return m_loopPaused; }
    void setLoopPaused(bool paused);
    QString liveImage() const;
    QString liveVideo() const;
    QString upcomingVideo() const;
    QString logPath() const;
    bool liveVideoLoop() const;
    bool liveVideoNext() const;
    QString liveText() const;
    QString liveBackground() const;
    QString liveReference() const;
    QVariantMap liveTemplate() const;
    QVariantMap nextTemplate() const;
    QString nextText() const;
    QString nextReference() const;
    QString nextBackground() const;
    bool blackout() const { return m_blackout; }
    void setBlackout(bool b);
    bool textHidden() const { return m_textHidden; }
    void setTextHidden(bool hidden);
    bool logo() const { return m_logo; }
    void setLogo(bool b);
    bool outputOn() const { return m_outputOn; }
    void setOutputOn(bool b);
    QString outputScreen() const { return m_outputScreen; }
    void setOutputScreen(const QString &s);
    QString defaultBackground() const { return m_defaultBg; }
    void setDefaultBackground(const QString &b);
    QStringList mediaFiles() const { return m_media; }
    QString lastSaved() const { return m_lastSaved; }

    Q_INVOKABLE void selectItem(int i);
    Q_INVOKABLE void goToSlide(int slide);   // within the current part
    Q_INVOKABLE void nextSlide();
    Q_INVOKABLE void prevSlide();
    Q_INVOKABLE void startService();
    // A part made of image slides (PowerPoint/PDF/pictures); loopSeconds > 0 makes it an announcement loop.
    Q_INVOKABLE int addSlidesItem(const QString &kind, const QString &title, const QStringList &images, int loopSeconds);
    Q_INVOKABLE int addVideo(const QUrl &file);
    Q_INVOKABLE void editVideo(int i, const QString &title, const QUrl &file, bool loop, bool next);
    Q_INVOKABLE bool fileExists(const QString &fileUrl) const;
    // Appends a line to the log file in the app data folder (kept small); used to find out why a video does not start.
    Q_INVOKABLE void log(const QString &message) const;
    Q_INVOKABLE void openLogFolder() const;
    Q_INVOKABLE int addItem(const QString &kind, const QString &title, const QString &subtitle, const QString &text);   // returns the new index
    Q_INVOKABLE void addSong(int songIndex);
    // Adds a Bible passage as a "scripture" part; returns its index or -1 when the reference is invalid.
    Q_INVOKABLE int addPassage(const QString &reference, int versesPerSlide, bool verseNumbers, bool show);
    // `order`: the order of the song parts for this part of the service ("Couplet 1, Refrein, Couplet 2, Refrein"), empty = as written
    Q_INVOKABLE void editItem(int i, const QString &title, const QString &subtitle, const QString &text, const QString &order = QString());
    // Everything the screen editor needs: {kind, title, subtitle, background, templateId, slides, refs}
    Q_INVOKABLE QVariantMap itemInfo(int i) const;
    // Replaces the slides of a part. References (bible) are kept only when keepRefs is set and the slide count is unchanged.
    Q_INVOKABLE void editItemSlides(int i, const QString &title, const QString &subtitle, const QStringList &slides,
                                    const QStringList &images, int loopSeconds, bool keepRefs);
    Q_INVOKABLE QStringList partNames(const QString &text) const { return ServiceModel::partNames(text); }   // names usable in an order
    // The slides that a song text gives in an order, as [{label, text}] (for the preview in the song editor).
    Q_INVOKABLE QVariantList songPreview(const QString &lyrics, const QString &order) const;
    Q_INVOKABLE QString itemOrder(int i) const;          // the order this part uses, as text
    Q_INVOKABLE QString itemDefaultOrder(int i) const;   // the standard order of the song
    Q_INVOKABLE QString itemText(int i) const;  // slides joined by blank lines (for the editor)
    Q_INVOKABLE void removeItem(int i);
    Q_INVOKABLE void moveItem(int i, int delta);
    Q_INVOKABLE void moveItemTo(int from, int to);     // drag and drop: the part ends up at index `to`
    Q_INVOKABLE void setItemBackground(int i, const QString &bg);
    Q_INVOKABLE void setItemTemplate(int i, const QString &templateId);  // "" = use the active template
    Q_INVOKABLE QString itemTemplate(int i) const;
    Q_INVOKABLE QString defaultTemplateName(int i) const;   // what the part uses when it has no template of its own
    Q_INVOKABLE void importMedia(const QUrl &file);
    // Removes an imported file from the library: deletes our copy (never anything outside the app's media folder)
    // and clears every reference to it (default background, part backgrounds).
    Q_INVOKABLE void removeMedia(const QString &url);
    Q_INVOKABLE QVariantMap mediaInfo(const QString &url) const;     // {name, kind: image|video, size, sizeText, exists}
    Q_INVOKABLE QVariantMap mediaUsage(const QString &url) const;    // {parts, isDefault}: where it is used
    Q_INVOKABLE void save();

signals:
    void serviceNameChanged();
    void liveChanged();
    void loopChanged();
    void videoStart();      // a video part just went live (also when it is selected again): start it from the beginning
    void slidesChanged();   // the list of slides of the current part changed (other part, or edited)
    void blackoutChanged();
    void logoChanged();
    void textHiddenChanged();
    void outputOnChanged();
    void outputScreenChanged();
    void defaultBackgroundChanged();
    void mediaFilesChanged();
    void savedChanged();

private:
    void load();
    void seedDefaultService();
    void scheduleSave();
    void onModelChanged();
    QString slideAt(int item, int slide) const;
    QString imageAt(int item, int slide) const;
    void updateLoop();              // (re)start or stop the loop timer for the live part
    void announceItemChange(bool otherItem);
    void noteLive();       // puts the live song in the usage report (once per time the output was open)
    QString refAt(int item, int slide) const;
    QVariantMap templateFor(int item) const;
    QString backgroundOf(int item) const;
    QString dataDir() const;

    ServiceModel m_service;
    SongLibrary m_songs;
    SongFilter m_songFilter;
    ExtraScreens m_extraScreens;
    UsageLog m_usage;
    QSet<QString> m_usedThisSession;   // songs already in the report since the output was opened
    Bible m_bible;
    TemplateLibrary m_templates;
    OverlayController m_overlays;
    Branding m_branding;
    SlideImporter m_importer;
    QString m_name = "Kerkdienst – Zondag";
    int m_item = 0, m_slide = 0;
    bool m_textHidden = false;
    bool m_blackout = false, m_logo = false, m_outputOn = false;
    QString m_outputScreen, m_defaultBg = "g:sunset", m_lastSaved;
    QStringList m_media;
    QTimer m_saveTimer;
    QTimer m_loopTimer;
    bool m_loopPaused = false;
};
