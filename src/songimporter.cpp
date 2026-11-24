#include "songimporter.h"
#include <QRegularExpression>
#include <QStringConverter>
#include <QStringList>

namespace SongImporter {

QString decode(const QByteArray &data) {
    if (auto enc = QStringConverter::encodingForData(data)) {   // looks for a BOM
        QStringDecoder dec(*enc);
        QString s = dec(data);
        if (!dec.hasError()) return s;
    }
    QStringDecoder utf8(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
    const QString s = utf8(data);
    if (!utf8.hasError()) return s;
    return QString::fromLatin1(data);
}

static QString normalise(QString t) {
    if (t.startsWith(QChar(0xFEFF))) t.remove(0, 1);
    t.replace("\r\n", "\n").replace('\r', '\n');
    return t;
}

// A line that only names a part of the song ("Verse 1", "Chorus", "Refrein", "[Bridge]", "Chorus (x2)"): returns the name, else "".
static QString sectionLabel(const QString &line) {
    static const QRegularExpression re(
        R"(^\s*[\[(]?\s*((?:verse|chorus|pre[- ]?chorus|bridge|intro|outro|tag|ending|interlude|refrain|couplet|refrein|brug|coda|instrumental|vers|slot|inleiding)(?:\s*\d+)?)\s*[\])]?\s*(\(?\s*x\s*\d+\s*\)?)?\s*:?\s*$)",
        QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(line);
    if (!m.hasMatch()) return {};
    QString name = m.captured(1).simplified();
    if (!m.captured(2).isEmpty()) name += " (" + m.captured(2).remove(QRegularExpression(R"([()\s])")).toLower() + ")";
    return name;
}

// Joins lines into slides. A blank line is the slide separator; the name of a part becomes a line "[Verse 1]" above
// its text (the song editor shows it, the output does not).
static QString lyricsFrom(const QStringList &lines) {
    QStringList out;
    QString pending;
    auto endBlock = [&] { if (!out.isEmpty() && !out.last().isEmpty()) out << QString(); };
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (const QString name = sectionLabel(line); !name.isEmpty()) { endBlock(); pending = name; continue; }
        if (line.isEmpty()) { endBlock(); continue; }
        if (!pending.isEmpty()) { out << "[" + pending + "]"; pending.clear(); }
        out << line;
    }
    while (!out.isEmpty() && out.last().isEmpty()) out.removeLast();
    return out.join('\n');
}

QList<ImportedSong> parseText(const QString &input) {
    const QStringList lines = normalise(input).split('\n');
    static const QRegularExpression ccliLine(R"(^\s*CCLI\s+(?:Song|Lied)\s*(?:#|nr\.?|nummer)?\s*(\d+))", QRegularExpression::CaseInsensitiveOption);

    int k = -1;
    ImportedSong song;
    for (int i = 0; i < lines.size(); ++i) {
        const auto m = ccliLine.match(lines[i]);
        if (m.hasMatch()) { k = i; song.ccli = m.captured(1); break; }
    }
    QStringList body = lines.mid(0, k >= 0 ? k : lines.size());
    if (k >= 0) {   // after the CCLI line: authors, copyright, licence text
        for (const QString &raw : lines.mid(k + 1)) {
            const QString l = raw.trimmed();
            if (l.isEmpty() || l.startsWith("For use solely", Qt::CaseInsensitive) || l.startsWith("CCLI License", Qt::CaseInsensitive)
                || l.startsWith("CCLI Licentie", Qt::CaseInsensitive) || l.startsWith("Voor gebruik", Qt::CaseInsensitive)) continue;
            if (l.startsWith(QChar(0x00A9)) || l.startsWith("(c)", Qt::CaseInsensitive)) {
                song.copyright += (song.copyright.isEmpty() ? "" : " ") + l;
            } else if (song.author.isEmpty() && song.copyright.isEmpty()) {
                song.author = l;
            }
        }
    }
    while (!body.isEmpty() && body.first().trimmed().isEmpty()) body.removeFirst();
    if (body.isEmpty()) return {};
    song.title = body.takeFirst().trimmed();
    song.author.replace(QRegularExpression(R"(\s*\|\s*)"), ", ");
    song.lyrics = lyricsFrom(body);
    if (!song.isValid()) return {};
    return {song};
}

QList<ImportedSong> parseUsr(const QString &input) {
    QList<ImportedSong> songs;
    const QStringList lines = normalise(input).split('\n');
    static const QRegularExpression header(R"(^\s*\[S\s*A?(\d*)\]\s*$)", QRegularExpression::CaseInsensitiveOption);
    ImportedSong cur;
    QString fields, words;
    bool inSong = false;

    auto finish = [&] {
        if (!inSong) return;
        const QStringList names = fields.split("/t"), parts = words.split("/t");
        QStringList blocks;
        for (int i = 0; i < parts.size(); ++i) {
            QString b = lyricsFrom(QString(parts[i]).replace("/n", "\n").split('\n'));
            if (b.isEmpty()) continue;
            const QString name = i < names.size() ? names[i].simplified() : QString();
            if (!name.isEmpty() && !b.startsWith('[')) b = "[" + name + "]\n" + b;   // the Fields line names each part
            blocks << b;
        }
        cur.lyrics = blocks.join("\n\n");
        cur.author.replace('/', ", ");
        if (cur.isValid()) songs << cur;
    };

    for (const QString &raw : lines) {
        const auto m = header.match(raw);
        if (m.hasMatch()) {
            finish();
            cur = ImportedSong(); fields.clear(); words.clear(); inSong = true;
            cur.ccli = m.captured(1);
            continue;
        }
        if (!inSong) continue;
        const int eq = raw.indexOf('=');
        if (eq < 0) continue;
        const QString key = raw.left(eq).trimmed().toLower(), value = raw.mid(eq + 1).trimmed();
        if (key == "title") cur.title = value;
        else if (key == "author") cur.author = value;
        else if (key == "copyright") cur.copyright = value;
        else if (key == "fields") fields = value;
        else if (key == "words") words = value;
    }
    finish();
    return songs;
}

QList<ImportedSong> parse(const QByteArray &data, const QString &fileName) {
    const QString text = normalise(decode(data));
    const bool usr = fileName.endsWith(".usr", Qt::CaseInsensitive) || text.trimmed().startsWith("[File]", Qt::CaseInsensitive);
    return usr ? parseUsr(text) : parseText(text);
}

}
