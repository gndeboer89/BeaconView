#pragma once
#include <QAbstractListModel>
#include <QJsonArray>
#include <QStringList>
#include <QVariantList>

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
    // Song parts in an order (see ServiceModel::expandOrder). `source` is the text with every part once ("[Verse 1]" lines);
    // `order` is the order this part of the service uses (empty = the parts as they are written); `defaultOrder` is the
    // song's own standard order, so "back to standard" is possible. `slides`/`labels` are the result of source + order.
    QString source;
    QStringList order, defaultOrder;
    // Credits of a song from the library, for the usage report.
    QString ccli, author;
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
    // Song order: a list of part names ("Verse 1", "Chorus", "#3" = the third part). Names are matched without regard to case.
    static QStringList parseOrder(const QString &text);              // "V1, Chorus; Verse 2" -> list (commas, semicolons, new lines)
    static QString joinOrder(const QStringList &order);
    // The slides of `source` in `order`. Empty order, or no name that matches, gives the parts as written.
    static void expandOrder(const QString &source, const QStringList &order, QStringList *slides, QStringList *labels);
    // The names that can be used in an order for this text, in the order they are written ("Verse 1", ..., "#3" for a part without a name).
    static QStringList partNames(const QString &source);
    // The slides that a song text gives in an order, as [{label, text}] (for the preview in the song editor).
    static QVariantList preview(const QString &lyrics, const QString &order);

private:
    QList<ServiceItem> m_items;
};
