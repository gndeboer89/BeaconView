#pragma once
#include <QByteArray>
#include <QList>
#include <QString>

// Reads song lyrics that were downloaded from SongSelect (CCLI): the plain-text lyrics file (.txt)
// and the USR file (.usr). Plain text files in a similar layout (title on the first line) work too.
// Only the lyrics and the credits are read; nothing is sent anywhere.
struct ImportedSong {
    QString title;
    QString author;
    QString copyright;
    QString ccli;       // CCLI song number, empty if the file has none
    QString lyrics;     // a blank line starts a new slide
    bool isValid() const { return !title.trimmed().isEmpty() && !lyrics.trimmed().isEmpty(); }
};

namespace SongImporter {
// Decodes the bytes (UTF-8 with or without BOM, UTF-16 with BOM, otherwise Latin-1).
QString decode(const QByteArray &data);
// Parses one file. The file name only helps to recognise the format; the content decides. Returns the songs found (usually one).
QList<ImportedSong> parse(const QByteArray &data, const QString &fileName);
QList<ImportedSong> parseText(const QString &text);
QList<ImportedSong> parseUsr(const QString &text);
}
