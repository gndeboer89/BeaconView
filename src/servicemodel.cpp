#include "servicemodel.h"
#include <QJsonObject>
#include <QRegularExpression>

QVariant ServiceModel::data(const QModelIndex &index, int role) const {
    const ServiceItem *it = item(index.row());
    if (!it) return {};
    switch (role) {
    case KindRole: return it->kind;
    case TitleRole: return it->title;
    case SubtitleRole: return it->subtitle;
    case SlideCountRole: return it->slides.size();
    case BackgroundRole: return it->background;
    case TemplateRole: return it->templateId;
    }
    return {};
}

QHash<int, QByteArray> ServiceModel::roleNames() const {
    return {{KindRole, "kind"}, {TitleRole, "title"}, {SubtitleRole, "subtitle"},
            {SlideCountRole, "slideCount"}, {BackgroundRole, "background"}, {TemplateRole, "templateId"}};
}

void ServiceModel::append(const ServiceItem &it) {
    beginInsertRows({}, m_items.size(), m_items.size());
    m_items.append(it);
    endInsertRows();
}

void ServiceModel::replace(int i, const ServiceItem &it) {
    if (!item(i)) return;
    m_items[i] = it;
    emit dataChanged(index(i), index(i));
}

void ServiceModel::remove(int i) {
    if (!item(i)) return;
    beginRemoveRows({}, i, i);
    m_items.removeAt(i);
    endRemoveRows();
}

bool ServiceModel::move(int from, int to) {
    if (!item(from) || !item(to) || from == to) return false;
    beginMoveRows({}, from, from, {}, to > from ? to + 1 : to);
    m_items.move(from, to);
    endMoveRows();
    return true;
}

void ServiceModel::setBackground(int i, const QString &bg) {
    if (!item(i)) return;
    m_items[i].background = bg;
    emit dataChanged(index(i), index(i), {BackgroundRole});
}

void ServiceModel::setTemplate(int i, const QString &id) {
    if (!item(i) || m_items[i].templateId == id) return;
    m_items[i].templateId = id;
    emit dataChanged(index(i), index(i), {TemplateRole});
}

QJsonArray ServiceModel::toJson() const {
    QJsonArray arr;
    for (const auto &it : m_items) {
        arr.append(QJsonObject{{"kind", it.kind}, {"title", it.title}, {"subtitle", it.subtitle},
                               {"background", it.background}, {"slides", QJsonArray::fromStringList(it.slides)},
                               {"refs", QJsonArray::fromStringList(it.refs)}, {"template", it.templateId}});
    }
    return arr;
}

void ServiceModel::fromJson(const QJsonArray &arr) {
    beginResetModel();
    m_items.clear();
    for (const auto &v : arr) {
        const QJsonObject o = v.toObject();
        ServiceItem it{o["kind"].toString("custom"), o["title"].toString(), o["subtitle"].toString(),
                       o["background"].toString(), {}, {}, o["template"].toString()};
        for (const auto &s : o["slides"].toArray()) it.slides << s.toString();
        for (const auto &s : o["refs"].toArray()) it.refs << s.toString();
        m_items.append(it);
    }
    endResetModel();
}

QStringList ServiceModel::splitSlides(const QString &text) {
    QStringList out;
    const auto blocks = text.trimmed().split(QRegularExpression("\\n\\s*\\n"), Qt::SkipEmptyParts);
    for (const QString &b : blocks) out << b.trimmed();
    return out;
}
