#pragma once
#include <QObject>
#include <QVariantList>
#include "songfilter.h"
#include "songlibrary.h"
#include "servicemodel.h"

// What the song page needs, for the song program. In the main program the page talks to the Presenter; the names are the same,
// but there is no service here: "add to service" does not exist (hasService is false).
class SongStudio : public QObject {
    Q_OBJECT
    Q_PROPERTY(SongLibrary *songs READ songs CONSTANT)
    Q_PROPERTY(SongFilter *songFilter READ songFilter CONSTANT)
    Q_PROPERTY(bool hasService READ hasService CONSTANT)
public:
    explicit SongStudio(QObject *parent = nullptr) : QObject(parent) { m_filter.setSourceModel(&m_songs); }
    SongLibrary *songs() { return &m_songs; }
    SongFilter *songFilter() { return &m_filter; }
    bool hasService() const { return false; }
    Q_INVOKABLE QStringList partNames(const QString &text) const { return ServiceModel::partNames(text); }
    Q_INVOKABLE QVariantList songPreview(const QString &lyrics, const QString &order) const { return ServiceModel::preview(lyrics, order); }
    Q_INVOKABLE bool openSongStudio() { return false; }
private:
    SongLibrary m_songs;
    SongFilter m_filter;
};
