#include "extrascreens.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

QStringList ExtraScreens::keys() {
    return {"id", "name", "kind", "screenName", "autoOpen", "on",
            "chromaColor", "textColor", "scale", "position", "showNext", "showClock", "showTimer"};
}

ExtraScreens::ExtraScreens(QObject *parent) : QAbstractListModel(parent) { load(); }

QHash<int, QByteArray> ExtraScreens::roleNames() const {
    QHash<int, QByteArray> r;
    const QStringList k = keys();
    for (int i = 0; i < k.size(); ++i) r[Qt::UserRole + 1 + i] = k[i].toUtf8();
    return r;
}

QVariant ExtraScreens::data(const QModelIndex &index, int role) const {
    const int k = role - Qt::UserRole - 1;
    if (index.row() < 0 || index.row() >= m_items.size() || k < 0 || k >= keys().size()) return {};
    return m_items[index.row()].value(keys()[k]);
}

bool ExtraScreens::setData(const QModelIndex &index, const QVariant &value, int role) {
    const int k = role - Qt::UserRole - 1;
    if (k < 0 || k >= keys().size()) return false;
    set(index.row(), keys()[k], value);
    return true;
}

QVariantMap ExtraScreens::defaults(const QString &kind) const {
    QVariantMap m{{"kind", kind}, {"screenName", QString()},   // "" = a loose window (to capture, or to drag to a screen)
                  {"autoOpen", true}, {"on", false}, {"scale", 1.0}};
    if (kind == "stage") {
        m.insert({{"showNext", true}, {"showClock", true}, {"showTimer", true}});
    } else {
        m.insert({{"chromaColor", "#00b140"}, {"textColor", "#ffffff"}, {"position", "bottom"}});
    }
    return m;
}

int ExtraScreens::add(const QString &kind) {
    if (kind != "stage" && kind != "subtitles") return -1;
    QVariantMap m = defaults(kind);
    const QString base = kind == "stage" ? QStringLiteral("Stage-scherm") : QStringLiteral("Ondertitels");
    int n = 0;
    for (const auto &it : std::as_const(m_items)) if (it["kind"] == kind) ++n;
    m["name"] = n == 0 ? base : QStringLiteral("%1 %2").arg(base).arg(n + 1);
    m["id"] = m_nextId++;
    beginInsertRows({}, m_items.size(), m_items.size());
    m_items.append(m);
    endInsertRows();
    emit countChanged();
    save();
    return m_items.size() - 1;
}

void ExtraScreens::remove(int i) {
    if (i < 0 || i >= m_items.size()) return;
    beginRemoveRows({}, i, i);
    m_items.removeAt(i);
    endRemoveRows();
    emit countChanged();
    save();
}

void ExtraScreens::set(int i, const QString &key, const QVariant &value) {
    const int k = keys().indexOf(key);
    if (i < 0 || i >= m_items.size() || k < 0 || key == "id" || key == "kind") return;
    QVariant v = value;
    if (key == "scale") v = qBound(0.5, value.toDouble(), 2.0);
    if (key == "position" && !QStringList{"bottom", "middle", "top"}.contains(value.toString())) return;
    if (key == "autoOpen" || key == "on" || key == "showNext" || key == "showClock" || key == "showTimer") v = value.toBool();
    if (m_items[i].value(key) == v) return;
    m_items[i][key] = v;
    emit dataChanged(index(i), index(i), {Qt::UserRole + 1 + k});
    if (key != "on") save();
}

QVariant ExtraScreens::get(int i, const QString &key) const {
    return (i >= 0 && i < m_items.size()) ? m_items[i].value(key) : QVariant();
}

void ExtraScreens::setOutputOn(bool on) {
    for (int i = 0; i < m_items.size(); ++i)
        if (m_items[i]["autoOpen"].toBool()) set(i, "on", on);
}

int ExtraScreens::openCount() const {
    int n = 0;
    for (const auto &it : m_items) n += it["on"].toBool();
    return n;
}

void ExtraScreens::load() {
    const auto doc = QJsonDocument::fromJson(QSettings().value("extraScreens").toByteArray());
    for (const auto &v : doc.array()) {
        const QJsonObject o = v.toObject();
        const QString kind = o["kind"].toString();
        if (kind != "stage" && kind != "subtitles") continue;
        QVariantMap m = defaults(kind);
        for (const QString &k : keys()) if (o.contains(k) && k != "on") m[k] = o[k].toVariant();
        m["on"] = false;
        if (!m.contains("id") || m["id"].toInt() <= 0) m["id"] = m_nextId;
        m_nextId = qMax(m_nextId, m["id"].toInt() + 1);
        m_items.append(m);
    }
}

void ExtraScreens::save() const {
    QJsonArray arr;
    for (const auto &it : m_items) {
        QJsonObject o;
        for (auto i = it.begin(); i != it.end(); ++i) if (i.key() != "on") o[i.key()] = QJsonValue::fromVariant(i.value());
        arr.append(o);
    }
    QSettings().setValue("extraScreens", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
