#pragma once
#include <QAbstractListModel>
#include <QJsonArray>
#include <QStringList>

struct ServiceItem {
    QString kind;        // welcome, song, prayer, scripture, sermon, offering, blessing, custom
    QString title;
    QString subtitle;
    QString background;  // "g:<id>" (gradient) or file URL; empty = default
    QStringList slides;
    QStringList refs;    // optional per-slide reference (e.g. "Psalm 23:1-2"), parallel to slides
    QString templateId;  // own template for this part; empty = use the active template
    // Image slides (PowerPoint/PDF/pictures): file URL per slide, "" = a normal text slide. Parallel to `slides`.
    QStringList images;
    int loopSeconds = 0; // > 0: slides advance by themselves every N seconds and wrap around (announcement loop)
    // Kind "video": the part is one video file.
    QString video;       // file URL
    bool videoLoop = false;   // start over when it ends
    bool videoNext = false;   // go to the next part when it ends
    // Song parts: the name of each slide ("Verse 1", "Chorus"), parallel to `slides` (may be shorter). Only for the operator and
    // the editor; never shown on the output.
    QStringList labels;
};

// Ordered list of parts ("onderdelen") that make up a church service.
class ServiceModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { KindRole = Qt::UserRole + 1, TitleRole, SubtitleRole, SlideCountRole, BackgroundRole, TemplateRole };

    explicit ServiceModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : m_items.size(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_items.size(); }
    const ServiceItem *item(int i) const { return (i >= 0 && i < m_items.size()) ? &m_items[i] : nullptr; }

    void append(const ServiceItem &it);
    void replace(int i, const ServiceItem &it);
    void remove(int i);
    bool move(int from, int to);
    void setBackground(int i, const QString &bg);
    void setTemplate(int i, const QString &id);

    QJsonArray toJson() const;
    void fromJson(const QJsonArray &arr);

    // Splits free text into slides: blank line = new slide.
    static QStringList splitSlides(const QString &text);
    // Like splitSlides, but a first line "[Verse 1]" in a block is the name of that slide: it goes to `labels` (one per slide,
    // empty when a slide has no name) and is not part of the slide text. joinLabeled is the reverse (for the editors).
    static QStringList splitLabeled(const QString &text, QStringList *labels);
    static QString joinLabeled(const QStringList &slides, const QStringList &labels);

private:
    QList<ServiceItem> m_items;
};
