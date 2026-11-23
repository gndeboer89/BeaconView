#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Slide templates (font, colours, alignment, ...). A few are built in and read-only;
// custom templates are stored as JSON in the app data folder. Exposed to QML as `presenter.templates`.
class TemplateLibrary : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList all READ all NOTIFY changed)
    Q_PROPERTY(QString activeId READ activeId NOTIFY activeChanged)
    Q_PROPERTY(QVariantMap active READ active NOTIFY activeChanged)
    Q_PROPERTY(QVariantMap kindDefaults READ kindDefaults NOTIFY kindDefaultsChanged)
public:
    explicit TemplateLibrary(QObject *parent = nullptr);

    QVariantList all() const;
    QString activeId() const { return m_activeId; }
    QVariantMap active() const { return get(m_activeId); }
    QVariantMap kindDefaults() const { return m_kindDefaults; }

    Q_INVOKABLE bool contains(const QString &id) const;
    Q_INVOKABLE QVariantMap get(const QString &id) const;      // falls back to the first built-in
    Q_INVOKABLE QString duplicate(const QString &id);          // returns the id of the new custom template
    Q_INVOKABLE void update(const QString &id, const QVariantMap &values);  // custom templates only
    Q_INVOKABLE void remove(const QString &id);                // custom templates only
    Q_INVOKABLE void setActive(const QString &id);
    // Default template per kind of part ("song", "scripture", ...). Empty id = no default (use the active template).
    Q_INVOKABLE QString kindTemplate(const QString &kind) const;   // "" when unset or when that template no longer exists
    Q_INVOKABLE void setKindTemplate(const QString &kind, const QString &id);

    static QVariantMap defaults();
    static QList<QVariantMap> builtins();

signals:
    void changed();
    void activeChanged();
    void kindDefaultsChanged();

private:
    static QVariantMap normalized(const QVariantMap &m);
    QString path() const;
    void load();
    void save() const;
    void saveKindDefaults() const;

    QList<QVariantMap> m_builtin;
    QList<QVariantMap> m_custom;
    QString m_activeId;
    QVariantMap m_kindDefaults;   // kind -> template id
};
