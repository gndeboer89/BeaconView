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
public:
    explicit TemplateLibrary(QObject *parent = nullptr);

    QVariantList all() const;
    QString activeId() const { return m_activeId; }
    QVariantMap active() const { return get(m_activeId); }

    Q_INVOKABLE QVariantMap get(const QString &id) const;      // falls back to the first built-in
    Q_INVOKABLE QString duplicate(const QString &id);          // returns the id of the new custom template
    Q_INVOKABLE void update(const QString &id, const QVariantMap &values);  // custom templates only
    Q_INVOKABLE void remove(const QString &id);                // custom templates only
    Q_INVOKABLE void setActive(const QString &id);

    static QVariantMap defaults();
    static QList<QVariantMap> builtins();

signals:
    void changed();
    void activeChanged();

private:
    static QVariantMap normalized(const QVariantMap &m);
    QString path() const;
    void load();
    void save() const;

    QList<QVariantMap> m_builtin;
    QList<QVariantMap> m_custom;
    QString m_activeId;
};
