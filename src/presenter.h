#pragma once
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include "bible.h"
#include "branding.h"
#include "overlaycontroller.h"
#include "servicemodel.h"
#include "songlibrary.h"
#include "templatelibrary.h"

// Central controller: owns the service, the live state shown on the output
// screen(s) and persistence. Exposed to QML as `presenter`.
class Presenter : public QObject {
    Q_OBJECT
    Q_PROPERTY(ServiceModel *service READ service CONSTANT)
    Q_PROPERTY(SongLibrary *songs READ songs CONSTANT)
    Q_PROPERTY(Bible *bible READ bible CONSTANT)
    Q_PROPERTY(TemplateLibrary *templates READ templates CONSTANT)
    Q_PROPERTY(OverlayController *overlays READ overlays CONSTANT)
    Q_PROPERTY(Branding *branding READ branding CONSTANT)
    Q_PROPERTY(QString serviceName READ serviceName WRITE setServiceName NOTIFY serviceNameChanged)
    Q_PROPERTY(int currentItem READ currentItem NOTIFY liveChanged)
    Q_PROPERTY(int currentSlide READ currentSlide NOTIFY liveChanged)
    Q_PROPERTY(int slideCount READ slideCount NOTIFY liveChanged)
    // All slides of the current part, so the operator can click any of them
    Q_PROPERTY(QStringList currentSlides READ currentSlides NOTIFY slidesChanged)
    Q_PROPERTY(QStringList currentRefs READ currentRefs NOTIFY slidesChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY slidesChanged)
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
    Q_PROPERTY(bool outputOn READ outputOn WRITE setOutputOn NOTIFY outputOnChanged)
    Q_PROPERTY(QString outputScreen READ outputScreen WRITE setOutputScreen NOTIFY outputScreenChanged)
    Q_PROPERTY(QString defaultBackground READ defaultBackground WRITE setDefaultBackground NOTIFY defaultBackgroundChanged)
    Q_PROPERTY(QStringList mediaFiles READ mediaFiles NOTIFY mediaFilesChanged)
    Q_PROPERTY(QString lastSaved READ lastSaved NOTIFY savedChanged)
public:
    explicit Presenter(QObject *parent = nullptr);

    ServiceModel *service() { return &m_service; }
    SongLibrary *songs() { return &m_songs; }
    Bible *bible() { return &m_bible; }
    TemplateLibrary *templates() { return &m_templates; }
    OverlayController *overlays() { return &m_overlays; }
    Branding *branding() { return &m_branding; }
    QString serviceName() const { return m_name; }
    void setServiceName(const QString &n);
    int currentItem() const { return m_item; }
    int currentSlide() const { return m_slide; }
    int slideCount() const;
    QStringList currentSlides() const;
    QStringList currentRefs() const;
    QString currentTitle() const;
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
    Q_INVOKABLE void addItem(const QString &kind, const QString &title, const QString &subtitle, const QString &text);
    Q_INVOKABLE void addSong(int songIndex);
    // Adds a Bible passage as a "scripture" part; returns its index or -1 when the reference is invalid.
    Q_INVOKABLE int addPassage(const QString &reference, int versesPerSlide, bool verseNumbers, bool show);
    Q_INVOKABLE void editItem(int i, const QString &title, const QString &subtitle, const QString &text);
    Q_INVOKABLE QString itemText(int i) const;  // slides joined by blank lines (for the editor)
    Q_INVOKABLE void removeItem(int i);
    Q_INVOKABLE void moveItem(int i, int delta);
    Q_INVOKABLE void setItemBackground(int i, const QString &bg);
    Q_INVOKABLE void setItemTemplate(int i, const QString &templateId);  // "" = use the active template
    Q_INVOKABLE QString itemTemplate(int i) const;
    Q_INVOKABLE QString defaultTemplateName(int i) const;   // what the part uses when it has no template of its own
    Q_INVOKABLE void importMedia(const QUrl &file);
    Q_INVOKABLE void save();

signals:
    void serviceNameChanged();
    void liveChanged();
    void slidesChanged();   // the list of slides of the current part changed (other part, or edited)
    void blackoutChanged();
    void logoChanged();
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
    QString refAt(int item, int slide) const;
    QVariantMap templateFor(int item) const;
    QString backgroundOf(int item) const;
    QString dataDir() const;

    ServiceModel m_service;
    SongLibrary m_songs;
    Bible m_bible;
    TemplateLibrary m_templates;
    OverlayController m_overlays;
    Branding m_branding;
    QString m_name = "Kerkdienst – Zondag";
    int m_item = 0, m_slide = 0;
    bool m_blackout = false, m_logo = false, m_outputOn = false;
    QString m_outputScreen, m_defaultBg = "g:sunset", m_lastSaved;
    QStringList m_media;
    QTimer m_saveTimer;
};
