#include "songlibrary.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

SongLibrary::SongLibrary(QObject *parent) : QAbstractListModel(parent) { load(); }

QString SongLibrary::path() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/songs.json";
}

QVariant SongLibrary::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_songs.size()) return {};
    const Song &s = m_songs[index.row()];
    switch (role) {
    case TitleRole: return s.title;
    case NumberRole: return s.number;
    case LyricsRole: return s.lyrics;
    }
    return {};
}

QHash<int, QByteArray> SongLibrary::roleNames() const {
    return {{TitleRole, "title"}, {NumberRole, "number"}, {LyricsRole, "lyrics"}};
}

void SongLibrary::add(const QString &title, const QString &number, const QString &lyrics) {
    beginInsertRows({}, m_songs.size(), m_songs.size());
    m_songs.append({title, number, lyrics});
    endInsertRows();
    save();
}

void SongLibrary::update(int i, const QString &title, const QString &number, const QString &lyrics) {
    if (i < 0 || i >= m_songs.size()) return;
    m_songs[i] = {title, number, lyrics};
    emit dataChanged(index(i), index(i));
    save();
}

void SongLibrary::remove(int i) {
    if (i < 0 || i >= m_songs.size()) return;
    beginRemoveRows({}, i, i);
    m_songs.removeAt(i);
    endRemoveRows();
    save();
}

void SongLibrary::load() {
    QFile f(path());
    if (f.open(QIODevice::ReadOnly)) {
        for (const auto &v : QJsonDocument::fromJson(f.readAll()).array()) {
            const QJsonObject o = v.toObject();
            m_songs.append({o["title"].toString(), o["number"].toString(), o["lyrics"].toString()});
        }
        return;
    }
    // First run: one public-domain example (Psalm 23, Statenvertaling).
    m_songs.append({"Psalm 23 (voorbeeld)", "",
        "De HEERE is mijn Herder,\nmij zal niets ontbreken.\n\n"
        "Hij doet mij nederliggen\nin grazige weiden;\n\n"
        "Hij voert mij zachtjes aan\nzeer stille wateren.\n\n"
        "Hij verkwikt mijn ziel;\nHij leidt mij in het spoor der gerechtigheid\nom Zijns Naams wil."});
    save();
}

void SongLibrary::save() const {
    QJsonArray arr;
    for (const Song &s : m_songs)
        arr.append(QJsonObject{{"title", s.title}, {"number", s.number}, {"lyrics", s.lyrics}});
    QFile f(path());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write(QJsonDocument(arr).toJson());
}
