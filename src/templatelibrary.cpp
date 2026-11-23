#include "templatelibrary.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>
#include <iterator>

QVariantMap TemplateLibrary::defaults() {
    return {{"name", QStringLiteral("Nieuw template")}, {"builtin", false},
            {"fontFamily", QString()},        // empty = system font
            {"fontSize", 108}, {"bold", true}, {"italic", false}, {"uppercase", false},
            {"textColor", QStringLiteral("#ffffff")},
            {"align", QStringLiteral("center")},     // left | center | right
            {"vAlign", QStringLiteral("middle")},    // top | middle | bottom
            {"lineHeight", 1.15}, {"margin", 140}, {"shadow", true},
            {"dim", 0.22},                            // how much the background is darkened (0..0.8)
            {"refColor", QStringLiteral("#f0f0f8")}, {"refSize", 56}, {"refItalic", true}};
}

QVariantMap TemplateLibrary::normalized(const QVariantMap &m) {
    QVariantMap n = defaults();
    for (auto it = m.begin(); it != m.end(); ++it) n[it.key()] = it.value();
    return n;
}

static QVariantMap make(const QString &id, const QString &name, const QVariantMap &over) {
    QVariantMap m = TemplateLibrary::defaults();
    for (auto it = over.begin(); it != over.end(); ++it) m[it.key()] = it.value();
    m["id"] = id;
    m["name"] = name;
    m["builtin"] = true;
    return m;
}

QList<QVariantMap> TemplateLibrary::builtins() {
    return {
        make("builtin:classic", "Klassiek", {}),
        make("builtin:modern", "Modern (links onder)",
             {{"align", "left"}, {"vAlign", "bottom"}, {"fontSize", 96}, {"margin", 160}, {"dim", 0.35}, {"refItalic", false}}),
        make("builtin:elegant", "Elegant (serif)",
             {{"fontFamily", "Georgia"}, {"bold", false}, {"fontSize", 104}, {"lineHeight", 1.25},
              {"textColor", "#fdf6e3"}, {"dim", 0.3}}),
        make("builtin:contrast", "Hoog contrast (geel)",
             {{"textColor", "#ffe45c"}, {"fontSize", 112}, {"dim", 0.5}}),
        make("builtin:caps", "Hoofdletters",
             {{"uppercase", true}, {"fontSize", 92}, {"lineHeight", 1.2}, {"dim", 0.3}}),
    };
}

TemplateLibrary::TemplateLibrary(QObject *parent) : QObject(parent), m_builtin(builtins()) {
    m_activeId = QSettings().value("activeTemplate", m_builtin.first()["id"]).toString();
    load();
    const QJsonObject kinds = QJsonDocument::fromJson(QSettings().value("kindTemplates").toString().toUtf8()).object();
    for (auto it = kinds.begin(); it != kinds.end(); ++it)
        if (contains(it.value().toString())) m_kindDefaults[it.key()] = it.value().toString();
    if (get(m_activeId)["id"].toString() != m_activeId) m_activeId = m_builtin.first()["id"].toString();
}

QString TemplateLibrary::path() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/templates.json";
}

void TemplateLibrary::load() {
    QFile f(path());
    if (!f.open(QIODevice::ReadOnly)) return;
    for (const auto &v : QJsonDocument::fromJson(f.readAll()).array()) {
        QVariantMap m = normalized(v.toObject().toVariantMap());
        if (m["id"].toString().isEmpty()) continue;
        m["builtin"] = false;
        m_custom.append(m);
    }
}

void TemplateLibrary::save() const {
    QJsonArray arr;
    for (const QVariantMap &m : m_custom) arr.append(QJsonObject::fromVariantMap(m));
    QFile f(path());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write(QJsonDocument(arr).toJson());
}

QVariantList TemplateLibrary::all() const {
    QVariantList l;
    for (const auto &m : m_builtin) l << m;
    for (const auto &m : m_custom) l << m;
    return l;
}

bool TemplateLibrary::contains(const QString &id) const {
    for (const auto &m : m_builtin) if (m["id"].toString() == id) return true;
    for (const auto &m : m_custom) if (m["id"].toString() == id) return true;
    return false;
}

QVariantMap TemplateLibrary::get(const QString &id) const {
    for (const auto &m : m_builtin) if (m["id"].toString() == id) return m;
    for (const auto &m : m_custom) if (m["id"].toString() == id) return m;
    return m_builtin.first();
}

QString TemplateLibrary::duplicate(const QString &id) {
    QVariantMap m = get(id);
    m["id"] = "custom:" + QUuid::createUuid().toString(QUuid::Id128).left(8);
    m["name"] = m["name"].toString() + QStringLiteral(" (kopie)");
    m["builtin"] = false;
    m_custom.append(m);
    save();
    emit changed();
    return m["id"].toString();
}

void TemplateLibrary::update(const QString &id, const QVariantMap &values) {
    for (QVariantMap &m : m_custom) {
        if (m["id"].toString() != id) continue;
        for (auto it = values.begin(); it != values.end(); ++it)
            if (it.key() != "id" && it.key() != "builtin") m[it.key()] = it.value();
        save();
        emit changed();
        if (id == m_activeId) emit activeChanged();
        return;
    }
}

void TemplateLibrary::remove(const QString &id) {
    for (int i = 0; i < m_custom.size(); ++i) {
        if (m_custom[i]["id"].toString() != id) continue;
        m_custom.removeAt(i);
        save();
        for (auto it = m_kindDefaults.begin(); it != m_kindDefaults.end();)   // forget defaults that pointed at it
            it = (it.value().toString() == id) ? m_kindDefaults.erase(it) : std::next(it);
        saveKindDefaults();
        emit kindDefaultsChanged();
        emit changed();
        if (id == m_activeId) setActive(m_builtin.first()["id"].toString());
        return;
    }
}

void TemplateLibrary::saveKindDefaults() const {
    QSettings().setValue("kindTemplates", QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(m_kindDefaults)).toJson(QJsonDocument::Compact)));
}

QString TemplateLibrary::kindTemplate(const QString &kind) const {
    const QString id = m_kindDefaults.value(kind).toString();
    return (!id.isEmpty() && contains(id)) ? id : QString();
}

void TemplateLibrary::setKindTemplate(const QString &kind, const QString &id) {
    if (kind.isEmpty()) return;
    if (id.isEmpty() || !contains(id)) {
        if (m_kindDefaults.remove(kind) == 0) return;
    } else {
        if (m_kindDefaults.value(kind).toString() == id) return;
        m_kindDefaults[kind] = id;
    }
    saveKindDefaults();
    emit kindDefaultsChanged();
}

void TemplateLibrary::setActive(const QString &id) {
    const QString real = get(id)["id"].toString();
    if (real == m_activeId) return;
    m_activeId = real;
    QSettings().setValue("activeTemplate", real);
    emit activeChanged();
}
