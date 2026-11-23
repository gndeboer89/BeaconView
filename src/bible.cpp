#include "bible.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <algorithm>

Bible::Bible(QObject *parent, const QString &resource) : QObject(parent), m_resource(resource) {}

void Bible::ensureLoaded() const {
    if (m_loaded) return;
    m_loaded = true;
    QFile f(m_resource);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    m_translation = root["translation"].toString();
    for (const auto &bv : root["books"].toArray()) {
        const QJsonObject o = bv.toObject();
        Book b{o["name"].toString(), o["ref"].toString(), {}, {}};
        for (const auto &a : o["aliases"].toArray()) b.aliases << a.toString();
        for (const auto &cv : o["chapters"].toArray()) {
            QStringList verses;
            for (const auto &v : cv.toArray()) verses << v.toString();
            b.chapters.append(verses);
        }
        m_books.append(b);
    }
}

QString Bible::normalize(const QString &s) {
    const QString d = s.toLower().normalized(QString::NormalizationForm_D);
    QString out;
    for (const QChar c : d) {
        if (c.category() == QChar::Mark_NonSpacing || c.isSpace() || c == '.' || c == '\'') continue;
        out += c;
    }
    return out;
}

QStringList Bible::bookNames() const {
    ensureLoaded();
    QStringList n;
    for (const Book &b : m_books) n << b.name;
    return n;
}
QString Bible::translation() const { ensureLoaded(); return m_translation; }
int Bible::bookCount() const { ensureLoaded(); return m_books.size(); }
QString Bible::bookName(int book) const { ensureLoaded(); return (book >= 0 && book < m_books.size()) ? m_books[book].name : QString(); }
int Bible::chapterCount(int book) const { ensureLoaded(); return (book >= 0 && book < m_books.size()) ? m_books[book].chapters.size() : 0; }

QString Bible::verseText(int book, int chapter, int verse) const {
    ensureLoaded();
    if (book < 0 || book >= m_books.size()) return {};
    const auto &chs = m_books[book].chapters;
    if (chapter < 1 || chapter > chs.size()) return {};
    return (verse >= 1 && verse <= chs[chapter - 1].size()) ? chs[chapter - 1][verse - 1] : QString();
}

int Bible::findBook(const QString &key) const {
    if (key.isEmpty()) return -1;
    for (int i = 0; i < m_books.size(); ++i)
        if (m_books[i].aliases.contains(key)) return i;       // exact alias / name
    if (key.size() < 3) return -1;
    int found = -1;
    for (int i = 0; i < m_books.size(); ++i) {                 // unique prefix
        for (const QString &a : m_books[i].aliases) {
            if (a.startsWith(key)) {
                if (found >= 0 && found != i) return -1;       // ambiguous
                found = i;
                break;
            }
        }
    }
    return found;
}

Bible::Passage Bible::parse(const QString &input) const {
    ensureLoaded();
    Passage p;
    static const QRegularExpression re(
        QStringLiteral(R"(^\s*([1-3]?\s*\.?\s*\p{L}[\p{L}\.\s]*?)\s*(?:(\d+)\s*(?:[:,]\s*([\d\s,\-–]+))?)?\s*$)"),
        QRegularExpression::UseUnicodePropertiesOption);
    const auto m = re.match(input);
    if (!m.hasMatch()) { p.error = QStringLiteral("Onbekende verwijzing"); return p; }

    p.book = findBook(normalize(m.captured(1)));
    if (p.book < 0) { p.error = QStringLiteral("Onbekend bijbelboek"); return p; }
    p.bookFound = true;
    if (m.captured(2).isEmpty()) { p.error = QStringLiteral("Geef een hoofdstuk op"); return p; }

    p.chapter = m.captured(2).toInt();
    const int maxCh = m_books[p.book].chapters.size();
    if (p.chapter < 1 || p.chapter > maxCh) {
        p.error = QStringLiteral("%1 heeft %2 hoofdstukken").arg(m_books[p.book].name).arg(maxCh);
        return p;
    }
    const int maxV = m_books[p.book].chapters[p.chapter - 1].size();

    const QString spec = m.captured(3).trimmed();
    if (spec.isEmpty()) {
        for (int v = 1; v <= maxV; ++v) p.verses << v;
        return p;
    }
    QList<int> verses;
    for (const QString &part : spec.split(',', Qt::SkipEmptyParts)) {
        const QStringList ends = part.trimmed().replace(QChar(0x2013), '-').split('-');
        bool ok1 = false, ok2 = true;
        const int a = ends[0].trimmed().toInt(&ok1);
        const int b = ends.size() > 1 ? ends[1].trimmed().toInt(&ok2) : a;
        if (!ok1 || !ok2 || ends.size() > 2 || a < 1 || b < a) { p.error = QStringLiteral("Ongeldig versbereik"); return p; }
        if (b > maxV) { p.error = QStringLiteral("%1 %2 heeft %3 verzen").arg(m_books[p.book].ref).arg(p.chapter).arg(maxV); return p; }
        for (int v = a; v <= b; ++v) verses << v;
    }
    std::sort(verses.begin(), verses.end());
    verses.erase(std::unique(verses.begin(), verses.end()), verses.end());
    p.verses = verses;
    return p;
}

QString Bible::verseRuns(const QList<int> &verses) const {
    QStringList runs;
    for (int i = 0; i < verses.size();) {
        int j = i;
        while (j + 1 < verses.size() && verses[j + 1] == verses[j] + 1) ++j;
        runs << (j > i ? QStringLiteral("%1-%2").arg(verses[i]).arg(verses[j]) : QString::number(verses[i]));
        i = j + 1;
    }
    return runs.join(',');
}

QString Bible::label(const Passage &p) const {
    ensureLoaded();
    if (p.book < 0 || p.verses.isEmpty()) return {};
    const QString base = QStringLiteral("%1 %2").arg(m_books[p.book].ref).arg(p.chapter);
    const int maxV = m_books[p.book].chapters[p.chapter - 1].size();
    if (p.verses.size() == maxV) return base;                  // whole chapter
    return base + ':' + verseRuns(p.verses);
}

QList<Bible::Slide> Bible::slides(const Passage &p, int versesPerSlide, bool numbers) const {
    ensureLoaded();
    QList<Slide> out;
    if (!p.ok()) return out;
    versesPerSlide = qMax(1, versesPerSlide);
    for (int i = 0; i < p.verses.size(); i += versesPerSlide) {
        const QList<int> chunk = p.verses.mid(i, versesPerSlide);
        QStringList texts;
        for (int v : chunk) {
            const QString t = verseText(p.book, p.chapter, v);
            texts << (numbers ? QStringLiteral("%1 %2").arg(v).arg(t) : t);
        }
        out.append({texts.join(' '),
                    QStringLiteral("%1 %2:%3").arg(m_books[p.book].ref).arg(p.chapter).arg(verseRuns(chunk))});
    }
    return out;
}

QVariantMap Bible::lookup(const QString &reference) const {
    const Passage p = parse(reference);
    QVariantMap r{{"ok", p.ok()}, {"bookFound", p.bookFound}, {"error", p.error}};
    if (p.ok()) {
        QVariantList verses;
        for (int v : p.verses) verses.append(QVariantMap{{"n", v}, {"text", verseText(p.book, p.chapter, v)}});
        r["verses"] = verses;
        r["label"] = label(p);
    }
    return r;
}

QVariantList Bible::search(const QString &query, int limit) const {
    ensureLoaded();
    QVariantList out;
    const QString q = query.trimmed();
    if (q.size() < 3) return out;
    for (const Book &b : m_books) {
        for (int c = 0; c < b.chapters.size(); ++c) {
            for (int v = 0; v < b.chapters[c].size(); ++v) {
                if (b.chapters[c][v].contains(q, Qt::CaseInsensitive)) {
                    out.append(QVariantMap{{"ref", QStringLiteral("%1 %2:%3").arg(b.ref).arg(c + 1).arg(v + 1)},
                                           {"text", b.chapters[c][v]}});
                    if (out.size() >= limit) return out;
                }
            }
        }
    }
    return out;
}
