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
};

// Ordered list of parts ("onderdelen") that make up a church service.
class ServiceModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { KindRole = Qt::UserRole + 1, TitleRole, SubtitleRole, SlideCountRole, BackgroundRole };

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

    QJsonArray toJson() const;
    void fromJson(const QJsonArray &arr);

    // Splits free text into slides: blank line = new slide.
    static QStringList splitSlides(const QString &text);

private:
    QList<ServiceItem> m_items;
};
