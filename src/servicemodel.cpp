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
                               {"refs", QJsonArray::fromStringList(it.refs)}, {"template", it.templateId},
                               {"images", QJsonArray::fromStringList(it.images)}, {"loopSeconds", it.loopSeconds},
                               {"video", it.video}, {"videoLoop", it.videoLoop}, {"videoNext", it.videoNext},
                               {"labels", QJsonArray::fromStringList(it.labels)}, {"source", it.source},
                               {"order", QJsonArray::fromStringList(it.order)}, {"defaultOrder", QJsonArray::fromStringList(it.defaultOrder)},
                               {"ccli", it.ccli}, {"author", it.author}});
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
        for (const auto &s : o["images"].toArray()) it.images << s.toString();
        for (const auto &s : o["labels"].toArray()) it.labels << s.toString();
        it.source = o["source"].toString();
        for (const auto &s : o["order"].toArray()) it.order << s.toString();
        for (const auto &s : o["defaultOrder"].toArray()) it.defaultOrder << s.toString();
        it.ccli = o["ccli"].toString();
        it.author = o["author"].toString();
        it.loopSeconds = o["loopSeconds"].toInt();
        it.video = o["video"].toString();
        it.videoLoop = o["videoLoop"].toBool();
        it.videoNext = o["videoNext"].toBool();
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

QStringList ServiceModel::splitLabeled(const QString &text, QStringList *labels) {
    static const QRegularExpression label(R"(^\[([^\]\n]{1,40})\][ \t]*(?:\n|$))");
    QStringList out;
    if (labels) labels->clear();
    const auto blocks = text.trimmed().split(QRegularExpression("\\n\\s*\\n"), Qt::SkipEmptyParts);
    for (const QString &raw : blocks) {
        QString b = raw.trimmed(), name;
        if (const auto m = label.match(b); m.hasMatch()) {
            name = m.captured(1).trimmed();
            b = b.mid(m.capturedLength()).trimmed();
            if (b.isEmpty()) continue;       // a name without text is not a slide
        }
        out << b;
        if (labels) *labels << name;
    }
    return out;
}

QString ServiceModel::joinLabeled(const QStringList &slides, const QStringList &labels) {
    QStringList blocks;
    for (int i = 0; i < slides.size(); ++i) {
        const QString name = i < labels.size() ? labels[i].trimmed() : QString();
        blocks << (name.isEmpty() ? slides[i] : "[" + name + "]\n" + slides[i]);
    }
    return blocks.join("\n\n");
}

QStringList ServiceModel::parseOrder(const QString &text) {
    QStringList out;
    for (const QString &p : text.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts)) {
        const QString t = p.simplified();
        if (!t.isEmpty()) out << t;
    }
    return out;
}

QString ServiceModel::joinOrder(const QStringList &order) { return order.join(", "); }

static QString foldName(const QString &s) { return s.simplified().toCaseFolded(); }

QStringList ServiceModel::partNames(const QString &source) {
    QStringList labels;
    splitLabeled(source, &labels);
    QStringList names;
    for (int i = 0; i < labels.size(); ++i) {
        const QString n = labels[i].isEmpty() ? "#" + QString::number(i + 1) : labels[i];
        if (!names.contains(n, Qt::CaseInsensitive)) names << n;     // a name that is used twice is one part
    }
    return names;
}

void ServiceModel::expandOrder(const QString &source, const QStringList &order, QStringList *slides, QStringList *labels) {
    QStringList parts, names;
    parts = splitLabeled(source, &names);
    slides->clear(); labels->clear();
    for (const QString &raw : order) {
        const QString want = foldName(raw);
        int found = -1;
        if (want.startsWith('#')) {
            bool ok = false;
            const int n = want.mid(1).toInt(&ok);
            if (ok && n >= 1 && n <= parts.size()) found = n - 1;
        } else {
            for (int i = 0; i < names.size() && found < 0; ++i) if (foldName(names[i]) == want) found = i;   // the first part with that name
        }
        if (found >= 0) { *slides << parts[found]; *labels << names[found]; }
    }
    if (slides->isEmpty()) { *slides = parts; *labels = names; }   // no order, or nothing in it matched
}

QVariantList ServiceModel::preview(const QString &lyrics, const QString &order) {
    QStringList slides, labels;
    expandOrder(lyrics, parseOrder(order), &slides, &labels);
    QVariantList out;
    for (int i = 0; i < slides.size(); ++i) out << QVariantMap{{"label", labels.value(i)}, {"text", slides[i]}};
    return out;
}
