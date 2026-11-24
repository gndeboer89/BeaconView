#pragma once
#include <QAbstractListModel>
#include <QVariantMap>

// Extra output screens next to the main output: a stage screen (current and next text, clock, timer for the people
// on stage) and a subtitle screen (the text on a chroma-key colour, for a video mixer or a stream).
// The list and its settings are stored; whether a screen is open ("on") is not.
class ExtraScreens : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    explicit ExtraScreens(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : m_items.size(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    // kind: "stage" or "subtitles". Returns the index of the new screen, -1 for an unknown kind.
    Q_INVOKABLE int add(const QString &kind);
    Q_INVOKABLE void remove(int i);
    // key is a role name ("name", "screenName", "chromaColor", "scale", ...)
    Q_INVOKABLE void set(int i, const QString &key, const QVariant &value);
    Q_INVOKABLE QVariant get(int i, const QString &key) const;
    // The main output opened or closed: the screens that open with it follow.
    void setOutputOn(bool on);
    int openCount() const;

    static QStringList keys();
signals:
    void countChanged();

private:
    QVariantMap defaults(const QString &kind) const;
    void load();
    void save() const;
    QList<QVariantMap> m_items;
    int m_nextId = 1;
};
